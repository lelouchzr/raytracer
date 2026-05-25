#include "rt/studio/StudioApp.hpp"
#include "rt/io/ResourcePaths.hpp"
#include "rt/math/Color.hpp"
#include "rt/studio/StudioPresets.hpp"
#include "rt/studio/StudioWidgets.hpp"
#ifdef RT_NATIVE_STUDIO
#include <SDL.h>
#include <backends/imgui_impl_sdl2.h>
#include <backends/imgui_impl_sdlrenderer2.h>
#include <cctype>
#include <fstream>
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>
#include <zlib.h>

namespace rt {
namespace {
constexpr auto panelFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar |
                            ImGuiWindowFlags_NoSavedSettings;

void panel(const char *name, float x, float y, float width, float height) {
    ImGui::SetNextWindowPos({x, y});
    ImGui::SetNextWindowSize({width, height});
    ImGui::Begin(name, nullptr, panelFlags);
}
} // namespace

StudioApp::StudioApp(const std::filesystem::path &file)
    : document_(file), folder_(std::filesystem::absolute(file).parent_path()) {
    try {
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0)
            throw std::runtime_error(SDL_GetError());
        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
        window_ = SDL_CreateWindow("Atelier - Raytracer Studio C++", SDL_WINDOWPOS_CENTERED,
                                   SDL_WINDOWPOS_CENTERED, 1440, 900,
                                   SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
        if (!window_)
            throw std::runtime_error(SDL_GetError());
        SDL_SetWindowMinimumSize(window_, 1000, 650);
        renderer_ =
            SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        if (!renderer_)
            renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_SOFTWARE);
        if (!renderer_)
            throw std::runtime_error(SDL_GetError());
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        StudioWidgets::configureInputs();
        auto &io = ImGui::GetIO();
        io.IniFilename = nullptr;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        bool loadedFont = false;
        for (const auto &font : {"/System/Library/Fonts/Supplemental/Arial.ttf",
                                 "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"})
            if (std::filesystem::exists(font)) {
                loadedFont = io.Fonts->AddFontFromFileTTF(font, 32.f) != nullptr;
                break;
            }
        if (!loadedFont) {
            ImFontConfig config;
            config.SizePixels = 32;
            io.Fonts->AddFontDefault(&config);
        }
        io.FontGlobalScale = .5f;
        ImGui::StyleColorsDark();
        auto &style = ImGui::GetStyle();
        style.WindowRounding = 0;
        style.FrameRounding = 4;
        style.WindowBorderSize = 1;
        style.FramePadding = {8, 6};
        style.ItemSpacing = {8, 10};
        style.WindowPadding = {16, 14};
        style.Colors[ImGuiCol_WindowBg] = {.12f, .135f, .15f, 1};
        style.Colors[ImGuiCol_FrameBg] = {.085f, .095f, .11f, 1};
        style.Colors[ImGuiCol_Button] = {.19f, .215f, .24f, 1};
        style.Colors[ImGuiCol_ButtonHovered] = {.27f, .30f, .33f, 1};
        style.Colors[ImGuiCol_Header] = {.32f, .27f, .22f, 1};
        style.Colors[ImGuiCol_CheckMark] = {.89f, .67f, .47f, 1};
        style.Colors[ImGuiCol_SliderGrab] = {.89f, .67f, .47f, 1};
        style.Colors[ImGuiCol_PlotHistogram] = style.Colors[ImGuiCol_SliderGrab];
        platformReady_ = ImGui_ImplSDL2_InitForSDLRenderer(window_, renderer_);
        rendererReady_ = ImGui_ImplSDLRenderer2_Init(renderer_);
        if (!platformReady_ || !rendererReady_)
            throw std::runtime_error("cannot initialize native GUI backends");
        const auto &materials = document_.data().at("materials").object();
        if (!materials.count(material_)) {
            selection_ = StudioSelection::Camera;
            material_ = materials.empty() ? "" : materials.begin()->first;
        }
        rendering_.restart(document_.data());
        lastRequest_ = std::chrono::steady_clock::now();
    } catch (...) {
        shutdown();
        throw;
    }
}

StudioApp::~StudioApp() {
    rendering_.cancel();
    shutdown();
}

void StudioApp::shutdown() noexcept {
    if (rendererReady_) {
        ImGui_ImplSDLRenderer2_Shutdown();
        rendererReady_ = false;
    }
    if (platformReady_) {
        ImGui_ImplSDL2_Shutdown();
        platformReady_ = false;
    }
    if (ImGui::GetCurrentContext())
        ImGui::DestroyContext();
    if (texture_) {
        SDL_DestroyTexture(texture_);
        texture_ = nullptr;
    }
    if (renderer_) {
        SDL_DestroyRenderer(renderer_);
        renderer_ = nullptr;
    }
    if (window_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }
    SDL_Quit();
}

void StudioApp::remember() {
    propertyEdit_ = 0;
    auto text = document_.data().dump();
    if (undo_.empty() || undo_.back() != text)
        undo_.push_back(std::move(text));
    if (undo_.size() > 100)
        undo_.erase(undo_.begin());
    redo_.clear();
}

void StudioApp::edit() {
    dirty_ = true;
    message_.clear();
}

void StudioApp::undo(bool forward) {
    propertyEdit_ = 0;
    auto &from = forward ? redo_ : undo_;
    auto &to = forward ? undo_ : redo_;
    if (from.empty())
        return;
    to.push_back(document_.data().dump());
    document_.replace(Json::parse(from.back()));
    from.pop_back();
    selection_ = StudioSelection::Camera;
    edit();
}

void StudioApp::header(float width) {
    panel("Toolbar", 0, 0, width, 56);
    ImGui::TextColored({.89f, .67f, .47f, 1}, "ATELIER");
    ImGui::SameLine();
    ImGui::TextDisabled("Raytracer Studio / C++");
    ImGui::SameLine();
    ImGui::TextUnformatted(document_.source().filename().string().c_str());
    ImGui::SameLine(std::max(400.f, width - 700.f));
    if (ImGui::IsKeyPressed(ImGuiKey_F4, false) &&
        !ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId))
        ImGui::SetKeyboardFocusHere();
    if (ImGui::Button("Ouvrir"))
        chooseFile(StudioFileAction::Open, "scene.json");
    ImGui::SameLine();
    if (ImGui::Button("Enregistrer"))
        chooseFile(StudioFileAction::Save, "scene.json");
    ImGui::SameLine();
    if (ImGui::Button("Exporter PNG"))
        chooseFile(StudioFileAction::Png, "render.png");
    ImGui::SameLine();
    if (ImGui::Button("Exporter HDR"))
        chooseFile(StudioFileAction::Hdr, "render.pfm");
    ImGui::SameLine();
    if (ImGui::Button("Annuler"))
        undo(false);
    ImGui::SameLine();
    if (ImGui::Button("Retablir"))
        undo(true);
    ImGui::End();
}

void StudioApp::tree(float height) {
    panel("Scene", 0, 56, 220, height - 80);
    ImGui::SeparatorText("SCENE");
    if (ImGui::IsKeyPressed(ImGuiKey_F1, false) &&
        !ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId))
        ImGui::SetKeyboardFocusHere();
    if (ImGui::Button("Charger un exemple", {-1, 0}))
        ImGui::OpenPopup("Examples");
    if (ImGui::BeginPopup("Examples")) {
        std::vector<std::filesystem::path> examples;
        for (const auto &entry : std::filesystem::directory_iterator(resourceRoot() / "gallery")) {
            auto name = entry.path().stem().string();
            // Numbered scenes are selectable; the catalog and render manifest are metadata.
            if (entry.path().extension() == ".json" && !name.empty() &&
                std::isdigit(static_cast<unsigned char>(name.front())))
                examples.push_back(entry.path());
        }
        std::sort(examples.begin(), examples.end());
        for (const auto &path : examples)
            if (ImGui::Selectable(path.stem().string().c_str())) {
                try {
                    auto replacement = StudioDocument(path);
                    remember();
                    document_ = std::move(replacement);
                    selection_ = StudioSelection::Camera;
                    edit();
                } catch (const std::exception &error) {
                    message_ = error.what();
                }
            }
        ImGui::EndPopup();
    }
    ImGui::SeparatorText("OBJETS");
    if (ImGui::SmallButton("+ Objet"))
        ImGui::OpenPopup("AddObject");
    if (ImGui::BeginPopup("AddObject")) {
        for (const auto &type : StudioPresets::objects())
            if (ImGui::Selectable(type.c_str()))
                addObject(type);
        ImGui::EndPopup();
    }
    auto &objects =
        std::get<Json::Array>(std::get<Json::Object>(document_.data().value).at("objects").value);
    for (std::size_t i = 0; i < objects.size(); ++i) {
        auto label = std::to_string(i + 1) + "  " + objects[i].at("type").string();
        if (ImGui::Selectable(label.c_str(), selection_ == StudioSelection::Object &&
                                                 selected_ == static_cast<int>(i))) {
            selection_ = StudioSelection::Object;
            selected_ = static_cast<int>(i);
        }
    }
    ImGui::SeparatorText("LUMIERES");
    if (ImGui::SmallButton("+ Lumiere"))
        ImGui::OpenPopup("AddLight");
    if (ImGui::BeginPopup("AddLight")) {
        for (const auto &type : StudioPresets::lights())
            if (ImGui::Selectable(type.c_str()))
                addLight(type);
        ImGui::EndPopup();
    }
    auto &lights =
        std::get<Json::Array>(std::get<Json::Object>(document_.data().value).at("lights").value);
    for (std::size_t i = 0; i < lights.size(); ++i) {
        auto label = lights[i].at("type").string() + " " + std::to_string(i + 1);
        if (ImGui::Selectable(label.c_str(), selection_ == StudioSelection::Light &&
                                                 selected_ == static_cast<int>(i))) {
            selection_ = StudioSelection::Light;
            selected_ = static_cast<int>(i);
        }
    }
    ImGui::SeparatorText("MATERIAUX");
    if (ImGui::SmallButton("+ Materiau"))
        addMaterial();
    for (const auto &[name, value] : document_.data().at("materials").object()) {
        (void)value;
        if (ImGui::Selectable(name.c_str(),
                              selection_ == StudioSelection::Material && material_ == name)) {
            selection_ = StudioSelection::Material;
            material_ = name;
        }
    }
    ImGui::Separator();
    if (ImGui::Selectable("Camera", selection_ == StudioSelection::Camera))
        selection_ = StudioSelection::Camera;
    if (ImGui::Selectable("Environnement HDR", selection_ == StudioSelection::Environment))
        selection_ = StudioSelection::Environment;
    if (ImGui::Selectable("Qualite et couleurs", selection_ == StudioSelection::Render))
        selection_ = StudioSelection::Render;
    if (ImGui::Button("Editeur JSON", {-1, 0})) {
        jsonError_.clear();
        jsonOpen_ = true;
    }
    ImGui::TextDisabled("Rendu local sur CPU");
    ImGui::End();
}

void StudioApp::run(int frameLimit, const std::filesystem::path &screenshot) {
    bool done = false, screenshotSaved = false;
    int frame = 0;
    while (!done) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT ||
                (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE))
                done = true;
        }
        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
        // Keep every panel reachable without a mouse, including on a remote desktop.
        if (!ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId)) {
            for (auto [key, name] :
                 {std::pair{ImGuiKey_F1, "Scene"}, std::pair{ImGuiKey_F2, "Properties"},
                  std::pair{ImGuiKey_F3, "View"}, std::pair{ImGuiKey_F4, "Toolbar"}})
                if (ImGui::IsKeyPressed(key, false))
                    ImGui::SetWindowFocus(name);
        }
        auto size = ImGui::GetIO().DisplaySize;
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{16, 8});
        header(size.x);
        ImGui::PopStyleVar();
        tree(size.y);
        inspector(size.x, size.y);
        updateImage();
        viewport(size.x, size.y);
        dialogs();
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{8, 3});
        panel("Footer", 0, size.y - 24, size.x, 24);
        ImGui::TextDisabled(
            "C++ / SDL2 / Dear ImGui    F1 Scene | F2 Proprietes | F3 Rendu | F4 Fichiers");
        ImGui::End();
        ImGui::PopStyleVar();
        if (dirty_ && live_ &&
            std::chrono::steady_clock::now() - lastRequest_ > std::chrono::milliseconds(150)) {
            rendering_.restart(document_.data());
            dirty_ = false;
            lastRequest_ = std::chrono::steady_clock::now();
        }
        ImGui::Render();
        SDL_RenderSetScale(renderer_, ImGui::GetIO().DisplayFramebufferScale.x,
                           ImGui::GetIO().DisplayFramebufferScale.y);
        SDL_SetRenderDrawColor(renderer_, 20, 23, 27, 255);
        SDL_RenderClear(renderer_);
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer_);
        auto state = rendering_.progress();
        if (frameLimit == 0 && !screenshot.empty() && !screenshotSaved && !state.running &&
            state.control && state.control->rows() == state.settings.height) {
            savePng(screenshot, true);
            screenshotSaved = true;
        }
        if (frameLimit > 0 && ++frame >= frameLimit) {
            if (!screenshot.empty())
                savePng(screenshot, true);
            done = true;
        }
        SDL_RenderPresent(renderer_);
        SDL_Delay(4);
    }
    rendering_.cancel();
}
} // namespace rt
#endif
