#include "rt/math/Color.hpp"
#include "rt/studio/StudioApp.hpp"
#include "rt/studio/StudioWidgets.hpp"
#ifdef RT_NATIVE_STUDIO
#include <SDL.h>
#include <fstream>
#include <imgui.h>
#include <zlib.h>

namespace rt {
void StudioApp::updateImage() {
    auto state = rendering_.progress();
    if (!state.control || state.control->rows() == 0)
        return;
    int rows = state.control->rows();
    if (state.generation == displayedGeneration_ && state.pass == displayedPass_ &&
        rows == displayedRows_)
        return;
    int width = state.settings.width, height = state.settings.height;
    if (width != imageWidth_ || height != imageHeight_) {
        std::vector<std::uint8_t> scaled(
            static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4, 255);
        for (int y = 0; y < height; ++y)
            for (int x = 0; x < width; ++x) {
                auto to = (static_cast<std::size_t>(y) * static_cast<std::size_t>(width) +
                           static_cast<std::size_t>(x)) *
                          4;
                if (!rgba_.empty()) {
                    int oldX = x * imageWidth_ / width, oldY = y * imageHeight_ / height;
                    auto from =
                        (static_cast<std::size_t>(oldY) * static_cast<std::size_t>(imageWidth_) +
                         static_cast<std::size_t>(oldX)) *
                        4;
                    for (std::size_t c = 0; c < 4; ++c)
                        scaled[to + c] = rgba_[from + c];
                } else {
                    scaled[to] = 24;
                    scaled[to + 1] = 27;
                    scaled[to + 2] = 31;
                }
            }
        rgba_ = std::move(scaled);
        imageWidth_ = width;
        imageHeight_ = height;
        if (texture_)
            SDL_DestroyTexture(texture_);
        texture_ = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING,
                                     width, height);
        if (!texture_)
            throw std::runtime_error(SDL_GetError());
    }
    auto mask = state.control->completedRows();
    auto pixels = state.control->snapshot();
    bool hdr = std::filesystem::path(state.settings.output).extension() == ".pfm";
    for (int y = 0; y < height; ++y)
        if (mask[static_cast<std::size_t>(y)])
            for (int x = 0; x < width; ++x) {
                auto index = static_cast<std::size_t>(y) * static_cast<std::size_t>(width) +
                             static_cast<std::size_t>(x);
                Vec3 color =
                    hdr ? toneMap(pixels[index] * state.settings.exposure, state.settings.toneMap)
                        : pixels[index];
                for (int c = 0; c < 3; ++c)
                    rgba_[index * 4 + static_cast<std::size_t>(c)] = static_cast<std::uint8_t>(
                        255 * displayChannel(color[c], state.settings.colorSpace) + .5);
                rgba_[index * 4 + 3] = 255;
            }
    if (SDL_UpdateTexture(texture_, nullptr, rgba_.data(), width * 4) != 0)
        throw std::runtime_error(SDL_GetError());
    displayedGeneration_ = state.generation;
    displayedPass_ = state.pass;
    displayedRows_ = rows;
}

void StudioApp::viewport(float width, float height) {
    ImGui::SetNextWindowPos({220, 56});
    ImGui::SetNextWindowSize({width - 550, height - 80});
    ImGui::Begin("View", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar);
    if (ImGui::IsKeyPressed(ImGuiKey_F3, false) &&
        !ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId))
        ImGui::SetKeyboardFocusHere();
    ImGui::Checkbox("Mise a jour automatique", &live_);
    ImGui::SameLine();
    if (ImGui::Button("Calculer")) {
        rendering_.restart(document_.data());
        dirty_ = false;
        lastRequest_ = std::chrono::steady_clock::now();
    }
    ImGui::SameLine();
    if (ImGui::Button("Pause")) {
        rendering_.cancel();
        live_ = false;
    }
    auto available = ImGui::GetContentRegionAvail();
    float imageAreaHeight = std::max(100.f, available.y - 125);
    ImGui::BeginChild("ImageArea", {0, imageAreaHeight}, false, ImGuiWindowFlags_NoScrollbar);
    if (texture_) {
        float scale = std::min(ImGui::GetContentRegionAvail().x / static_cast<float>(imageWidth_),
                               ImGui::GetContentRegionAvail().y / static_cast<float>(imageHeight_));
        ImVec2 imageSize{static_cast<float>(imageWidth_) * scale,
                         static_cast<float>(imageHeight_) * scale};
        auto offset = ImGui::GetContentRegionAvail();
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (offset.x - imageSize.x) * .5f);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (offset.y - imageSize.y) * .5f);
        ImGui::Image(reinterpret_cast<ImTextureID>(texture_), imageSize);
        if (ImGui::IsItemHovered()) {
            try {
                auto &io = ImGui::GetIO();
                auto &camera = std::get<Json::Object>(document_.data().value).at("camera");
                Vec3 position = camera.vector("position", {0, 2, 7}),
                     target = camera.vector("target", {0, 1, 0}), delta = position - target;
                double radius = std::sqrt(delta.length2());
                bool moved = false;
                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                    remember();
                if (ImGui::IsMouseDragging(ImGuiMouseButton_Left) && radius > 1e-8) {
                    if (io.KeyShift) {
                        auto forward = unit(-delta),
                             right = unit(cross(forward, camera.vector("up", {0, 1, 0}))),
                             up = cross(right, forward);
                        auto shift = right * (-io.MouseDelta.x * radius * .0015) +
                                     up * (io.MouseDelta.y * radius * .0015);
                        position += shift;
                        target += shift;
                    } else {
                        double theta = std::atan2(delta.x, delta.z) - io.MouseDelta.x * .008,
                               phi = std::clamp(std::acos(std::clamp(delta.y / radius, -1., 1.)) +
                                                    io.MouseDelta.y * .008,
                                                .03, pi - .03);
                        position = target + Vec3{radius * std::sin(phi) * std::sin(theta),
                                                 radius * std::cos(phi),
                                                 radius * std::sin(phi) * std::cos(theta)};
                    }
                    moved = true;
                }
                if (io.MouseWheel != 0 && radius > 1e-8) {
                    remember();
                    double next = std::clamp(radius * std::exp(-io.MouseWheel * .12), .1, 1e6);
                    position = target + delta * (next / radius);
                    moved = true;
                }
                if (moved) {
                    std::get<Json::Object>(camera.value)["position"] = StudioWidgets::vec(position);
                    std::get<Json::Object>(camera.value)["target"] = StudioWidgets::vec(target);
                    edit();
                }

            } catch (const std::exception &error) {
                message_ = error.what();
            }
        }
    } else {
        ImGui::TextWrapped("Preparation de la scene. Le premier apercu arrive pendant que le "
                           "moteur calcule l'image.");
    }
    ImGui::EndChild();
    auto state = rendering_.progress();
    ImGui::TextDisabled("Glisser : orbite | Maj + glisser : deplacer | Molette : zoom");
    int complete = state.control ? state.control->rows() : 0;
    float fraction = state.settings.height > 0
                         ? static_cast<float>(complete) / static_cast<float>(state.settings.height)
                         : 0;
    ImGui::ProgressBar(fraction, {-1, 4}, "");
    ImGui::Text("%s | %d x %d | %d ech./pixel | passe %d | %.2f s",
                !state.error.empty()                          ? "Erreur de scene"
                : state.running                               ? "Calcul en cours"
                : dirty_                                      ? "Modifications en attente"
                : state.control && state.control->cancelled() ? "En pause"
                : complete == state.settings.height           ? "Rendu termine"
                                                              : "En pause",
                state.settings.width, state.settings.height, state.settings.samples, state.pass,
                state.seconds);
    auto error = state.error.empty() ? message_ : state.error;
    if (!error.empty())
        ImGui::TextWrapped("%s", error.c_str());
    ImGui::End();
}

void StudioApp::savePng(const std::filesystem::path &path, bool wholeWindow) {
    int width = imageWidth_, height = imageHeight_;
    std::vector<std::uint8_t> pixels;
    if (wholeWindow) {
        if (SDL_GetRendererOutputSize(renderer_, &width, &height) != 0)
            throw std::runtime_error(SDL_GetError());
        pixels.resize(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4);
        if (SDL_RenderReadPixels(renderer_, nullptr, SDL_PIXELFORMAT_RGBA32, pixels.data(),
                                 width * 4) != 0)
            throw std::runtime_error(SDL_GetError());
    } else
        pixels = rgba_;
    if (width < 1 || height < 1 || pixels.empty())
        throw std::runtime_error("aucune image a exporter");
    std::vector<unsigned char> scanlines;
    scanlines.reserve(static_cast<std::size_t>(height) * (static_cast<std::size_t>(width) * 4 + 1));
    for (int y = 0; y < height; ++y) {
        scanlines.push_back(0);
        auto start = static_cast<std::size_t>(y) * static_cast<std::size_t>(width) * 4;
        scanlines.insert(scanlines.end(), pixels.begin() + static_cast<std::ptrdiff_t>(start),
                         pixels.begin() + static_cast<std::ptrdiff_t>(
                                              start + static_cast<std::size_t>(width) * 4));
    }
    uLongf bound = compressBound(static_cast<uLong>(scanlines.size()));
    std::vector<unsigned char> compressed(bound);
    if (compress2(compressed.data(), &bound, scanlines.data(), static_cast<uLong>(scanlines.size()),
                  4) != Z_OK)
        throw std::runtime_error("PNG compression failed");
    compressed.resize(bound);
    auto temporary = path.string() + ".tmp." +
                     std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    try {
        std::ofstream output(temporary, std::ios::binary);
        if (!output)
            throw std::runtime_error("cannot write PNG");
        auto integer = [&](std::uint32_t value) {
            for (int i = 24; i >= 0; i -= 8)
                output.put(static_cast<char>((value >> i) & 255));
        };
        auto chunk = [&](const char *name, const std::vector<unsigned char> &data) {
            integer(static_cast<std::uint32_t>(data.size()));
            output.write(name, 4);
            output.write(reinterpret_cast<const char *>(data.data()),
                         static_cast<std::streamsize>(data.size()));
            uLong checksum = crc32(0, reinterpret_cast<const Bytef *>(name), 4);
            // A null buffer resets zlib CRC state; an empty IEND chunk still has a type CRC.
            if (!data.empty())
                checksum = crc32(checksum, data.data(), static_cast<uInt>(data.size()));
            integer(static_cast<std::uint32_t>(checksum));
        };
        output.write("\x89PNG\r\n\x1a\n", 8);
        std::vector<unsigned char> header;
        for (auto value : {static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height)})
            for (int i = 24; i >= 0; i -= 8)
                header.push_back(static_cast<unsigned char>((value >> i) & 255));
        header.insert(header.end(), {8, 6, 0, 0, 0});
        chunk("IHDR", header);
        chunk("IDAT", compressed);
        chunk("IEND", {});
        output.flush();
        if (!output)
            throw std::runtime_error("PNG write failed");
        output.close();
        if (!output)
            throw std::runtime_error("PNG close failed");
        std::filesystem::rename(temporary, path);
    } catch (...) {
        std::error_code error;
        std::filesystem::remove(temporary, error);
        throw;
    }
}
} // namespace rt
#endif
