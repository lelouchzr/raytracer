#include "rt/studio/StudioApp.hpp"
#include "rt/studio/StudioPresets.hpp"
#include "rt/studio/StudioWidgets.hpp"
#ifdef RT_NATIVE_STUDIO
#include <functional>
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

namespace rt {
using W = StudioWidgets;

void StudioApp::addObject(const std::string &type) {
    if (document_.data().at("materials").object().empty())
        addMaterial();
    remember();
    auto &list =
        std::get<Json::Array>(std::get<Json::Object>(document_.data().value).at("objects").value);
    list.push_back(
        StudioPresets::object(type, document_.data().at("materials").object().begin()->first));
    selected_ = static_cast<int>(list.size() - 1);
    selection_ = StudioSelection::Object;
    edit();
}

void StudioApp::addLight(const std::string &type) {
    remember();
    auto &list =
        std::get<Json::Array>(std::get<Json::Object>(document_.data().value).at("lights").value);
    list.push_back(StudioPresets::light(type));
    selected_ = static_cast<int>(list.size() - 1);
    selection_ = StudioSelection::Light;
    edit();
}

void StudioApp::addMaterial() {
    remember();
    auto &materials = std::get<Json::Object>(
        std::get<Json::Object>(document_.data().value).at("materials").value);
    int i = 1;
    while (materials.count("material_" + std::to_string(i)))
        ++i;
    material_ = "material_" + std::to_string(i);
    materials[material_] = Json(Json::Object{{"type", Json(std::string("pbr"))},
                                             {"color", W::vec({.5, .5, .5})},
                                             {"roughness", Json(.3)}});
    selection_ = StudioSelection::Material;
    edit();
}

void StudioApp::duplicate() {
    auto *source = selectedNode();
    if (!source)
        return;
    auto copy = *source;
    remember();
    if (selection_ == StudioSelection::Material) {
        auto &materials = std::get<Json::Object>(
            std::get<Json::Object>(document_.data().value).at("materials").value);
        auto name = material_ + "_copy";
        while (materials.count(name))
            name += "_copy";
        materials[name] = std::move(copy);
        material_ = name;
    } else {
        auto &list = std::get<Json::Array>(
            std::get<Json::Object>(document_.data().value)
                .at(selection_ == StudioSelection::Object ? "objects" : "lights")
                .value);
        if (copy.has("translate")) {
            auto p = copy.at("translate").vector();
            p.x += 1;
            std::get<Json::Object>(copy.value)["translate"] = W::vec(p);
        }
        list.push_back(std::move(copy));
        selected_ = static_cast<int>(list.size() - 1);
    }
    edit();
}

void StudioApp::remove() {
    if (selection_ == StudioSelection::Material) {
        bool used = false;
        std::function<void(const Json &)> inspect = [&](const Json &value) {
            if (auto object = std::get_if<Json::Object>(&value.value)) {
                if (object->count("material") && object->at("material").string() == material_)
                    used = true;
                for (const auto &[key, child] : *object) {
                    (void)key;
                    inspect(child);
                }
            } else if (auto array = std::get_if<Json::Array>(&value.value))
                for (const auto &child : *array)
                    inspect(child);
        };
        inspect(document_.data().at("objects"));
        if (used) {
            message_ = "Ce materiau est utilise par un objet.";
            return;
        }
        remember();
        std::get<Json::Object>(std::get<Json::Object>(document_.data().value).at("materials").value)
            .erase(material_);
    } else {
        auto &list = std::get<Json::Array>(
            std::get<Json::Object>(document_.data().value)
                .at(selection_ == StudioSelection::Object ? "objects" : "lights")
                .value);
        if (selection_ == StudioSelection::Object && list.size() == 1) {
            message_ = "Conserver au moins un objet.";
            return;
        }
        if (selected_ < 0 || static_cast<std::size_t>(selected_) >= list.size())
            return;
        remember();
        list.erase(list.begin() + selected_);
    }
    selection_ = StudioSelection::Camera;
    edit();
}

void StudioApp::chooseFile(StudioFileAction action, const std::string &filename) {
    fileAction_ = action;
    filename_ = filename;
    overwrite_ = false;
}

void StudioApp::applyFile() {
    auto path = std::filesystem::absolute(folder_ / filename_);
    if (fileAction_ == StudioFileAction::Open) {
        auto replacement = StudioDocument(path);
        remember();
        document_ = std::move(replacement);
        selection_ = StudioSelection::Camera;
        edit();
    } else if (fileAction_ == StudioFileAction::Save) {
        document_.save(path);
        message_ = "Scene enregistree : " + path.string();
    } else if (fileAction_ == StudioFileAction::Png) {
        savePng(path);
        message_ = "PNG exporte : " + path.string();
    } else if (fileAction_ == StudioFileAction::Hdr) {
        if (path.extension() != ".pfm")
            throw std::runtime_error("choisir une extension .pfm pour l'export HDR");
        rendering_.restart(document_.data(), path);
        dirty_ = false;
        message_ = "Export HDR : " + path.string();
    } else {
        Json *node = selectedNode();
        if (fileAction_ == StudioFileAction::Environment)
            node = &W::member(document_.data(), "environment", Json(Json::Object{}));
        if (!node)
            throw std::runtime_error("selection absente");
        remember();
        auto key = fileAction_ == StudioFileAction::Normal ? "normalFile" : "file";
        W::member(*node, key, Json(path.string())) = Json(path.string());
        edit();
    }
    fileAction_ = StudioFileAction::None;
    overwrite_ = false;
}

void StudioApp::fileDialog() {
    if (fileAction_ == StudioFileAction::None)
        return;
    ImGui::OpenPopup("Fichier");
    ImGui::SetNextWindowSize({720, 560}, ImGuiCond_FirstUseEver);
    if (ImGui::BeginPopupModal("Fichier", nullptr, ImGuiWindowFlags_NoSavedSettings)) {
        ImGui::TextWrapped("Dossier : %s", folder_.string().c_str());
        if (ImGui::Button("Dossier parent")) {
            folder_ = folder_.parent_path();
            overwrite_ = false;
        }
        // Reserve room for the filename, overwrite prompt, actions and error message.
        float listHeight = std::max(80.f, ImGui::GetContentRegionAvail().y - 180.f);
        ImGui::BeginChild("Files", {0, listHeight}, ImGuiChildFlags_Borders);
        std::vector<std::filesystem::directory_entry> entries;
        std::error_code error;
        for (std::filesystem::directory_iterator it(folder_, error), end; !error && it != end;
             it.increment(error))
            entries.push_back(*it);
        std::sort(entries.begin(), entries.end(), [](const auto &a, const auto &b) {
            return a.path().filename() < b.path().filename();
        });
        for (const auto &entry : entries) {
            bool dir = entry.is_directory(error);
            auto label = std::string(dir ? "[D] " : "    ") + entry.path().filename().string();
            if (ImGui::Selectable(label.c_str(), filename_ == entry.path().filename(),
                                  ImGuiSelectableFlags_AllowDoubleClick)) {
                if (dir) {
                    folder_ = entry.path();
                    filename_.clear();
                    overwrite_ = false;
                    break;
                }
                if (filename_ != entry.path().filename())
                    overwrite_ = false;
                filename_ = entry.path().filename().string();
            }
        }
        ImGui::EndChild();
        if (ImGui::InputText("Nom du fichier", &filename_))
            overwrite_ = false;
        bool saving = fileAction_ == StudioFileAction::Save ||
                      fileAction_ == StudioFileAction::Png || fileAction_ == StudioFileAction::Hdr;
        if (overwrite_)
            ImGui::TextColored({1, .7f, .5f, 1},
                               "Le fichier existe. Valider a nouveau pour le remplacer.");
        ImGui::BeginDisabled(filename_.empty());
        if (ImGui::Button(saving ? "Enregistrer / exporter" : "Ouvrir")) {
            auto path = folder_ / filename_;
            if (saving && std::filesystem::exists(path) && !overwrite_)
                overwrite_ = true;
            else
                try {
                    applyFile();
                    ImGui::CloseCurrentPopup();
                } catch (const std::exception &e) {
                    message_ = e.what();
                }
        }
        ImGui::EndDisabled();
        ImGui::SameLine();
        if (ImGui::Button("Annuler")) {
            fileAction_ = StudioFileAction::None;
            overwrite_ = false;
            ImGui::CloseCurrentPopup();
        }
        if (!message_.empty())
            ImGui::TextWrapped("%s", message_.c_str());
        ImGui::EndPopup();
    }
}

void StudioApp::dialogs() {
    fileDialog();
    if (jsonOpen_) {
        // The inspector has committed this frame's edits before the JSON snapshot is taken.
        jsonText_ = StudioDocument::formatted(document_.data());
        ImGui::OpenPopup("Scene JSON");
        jsonOpen_ = false;
    }
    ImGui::SetNextWindowSize({850, 650}, ImGuiCond_FirstUseEver);
    if (ImGui::BeginPopupModal("Scene JSON", nullptr, ImGuiWindowFlags_NoSavedSettings)) {
        ImGui::TextWrapped(
            "Acces a tous les champs, groupes, matrices et imports du format de scene.");
        ImGui::InputTextMultiline("##document", &jsonText_, {-1, 480});
        if (ImGui::Button("Appliquer")) {
            try {
                auto json = Json::parse(jsonText_);
                auto replacement = document_;
                replacement.replace(std::move(json));
                remember();
                document_ = std::move(replacement);
                selection_ = StudioSelection::Camera;
                edit();
                ImGui::CloseCurrentPopup();
            } catch (const std::exception &e) {
                jsonError_ = e.what();
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Fermer"))
            ImGui::CloseCurrentPopup();
        if (!jsonError_.empty())
            ImGui::TextWrapped("%s", jsonError_.c_str());
        ImGui::EndPopup();
    }
}
} // namespace rt
#endif
