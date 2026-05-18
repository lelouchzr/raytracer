#include "rt/io/ResourcePaths.hpp"
#include "rt/studio/StudioApp.hpp"
#include "rt/studio/StudioPresets.hpp"
#include "rt/studio/StudioWidgets.hpp"
#ifdef RT_NATIVE_STUDIO
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

namespace rt {
using W = StudioWidgets;

Json *StudioApp::selectedNode() {
    auto &root = std::get<Json::Object>(document_.data().value);
    if (selection_ == StudioSelection::Material) {
        auto &m = std::get<Json::Object>(root.at("materials").value);
        auto it = m.find(material_);
        return it == m.end() ? nullptr : &it->second;
    }
    if (selection_ == StudioSelection::Object || selection_ == StudioSelection::Light) {
        auto &list = std::get<Json::Array>(
            root.at(selection_ == StudioSelection::Object ? "objects" : "lights").value);
        return selected_ >= 0 && static_cast<std::size_t>(selected_) < list.size()
                   ? &list[static_cast<std::size_t>(selected_)]
                   : nullptr;
    }
    if (selection_ == StudioSelection::Camera)
        return &root.at("camera");
    if (selection_ == StudioSelection::Render)
        return &W::member(document_.data(), "render", Json(Json::Object{}));
    return root.count("environment") ? &root.at("environment") : nullptr;
}

void StudioApp::objectProperties(Json &node) {
    auto type = node.at("type").string();
    if (W::combo(node, "type", "Primitive", StudioPresets::objects(), type)) {
        auto &materials = document_.data().at("materials").object();
        auto material =
            node.string("material", materials.empty() ? "material" : materials.begin()->first);
        auto position = node.vector("translate", {});
        node = StudioPresets::object(node.at("type").string(), material);
        W::member(node, "translate", W::vec(position)) = W::vec(position);
        type = node.at("type").string();
    }
    if (type != "group") {
        std::vector<std::string> names;
        for (const auto &[name, value] : document_.data().at("materials").object()) {
            (void)value;
            names.push_back(name);
        }
        if (!names.empty())
            W::combo(node, "material", "Materiau", names, names.front());
    }
    W::vector(node, "translate", "Position", {});
    W::vector(node, "rotate", "Rotation (degres)", {});
    W::vector(node, "scale", "Echelle", {1, 1, 1});
    W::vector(node, "shear", "Cisaillement", {});
    if (type == "sphere" || type == "cylinder" || type == "cone" || type == "limited_cylinder" ||
        type == "limited_cone")
        W::number(node, "radius", "Rayon", 1, .000001, 1000000);
    if (type == "limited_cylinder" || type == "limited_cone")
        W::number(node, "height", "Hauteur", 2, .000001, 1000000);
    if (type == "plane")
        W::vector(node, "normal", "Normale", {0, 1, 0});
    if (type == "box")
        W::vector(node, "size", "Dimensions", {1, 1, 1});
    if (type == "triangle") {
        W::vector(node, "a", "Sommet A", {-1, 0, 0});
        W::vector(node, "b", "Sommet B", {1, 0, 0});
        W::vector(node, "c", "Sommet C", {0, 2, 0});
    }
    if (type == "torus") {
        W::number(node, "major", "Rayon majeur", 1.3, .1, 10);
        W::number(node, "minor", "Rayon mineur", .35, .01, 2);
    }
    if (type == "fractal")
        W::number(node, "iterations", "Iterations", 12, 2, 24, true);
    if (type == "mobius") {
        W::number(node, "radius", "Rayon", 1.5, .1, 10);
        W::number(node, "width", "Demi-largeur", .5, .01, 2);
        W::number(node, "segments", "Segments", 128, 16, 1024, true);
    }
    if (type == "obj") {
        W::text(node, "file", "Maillage OBJ");
        if (ImGui::Button("Parcourir OBJ"))
            chooseFile(StudioFileAction::Mesh, "");
    }
    if (type == "group")
        ImGui::TextWrapped(
            "Les enfants du groupe et les matrices sont editables dans l'editeur JSON.");
}

void StudioApp::materialProperties(Json &node) {
    auto old = node.string("type", "flat");
    if (W::combo(node, "type", "Modele", {"pbr", "phong", "flat"}, old)) {
        auto color = node.vector("color", {.7, .7, .7});
        node =
            Json(Json::Object{{"type", Json(node.at("type").string())}, {"color", W::vec(color)}});
    }
    W::color(node, "color", "Couleur de base", {.7, .7, .7});
    auto type = node.string("type", "flat");
    if (type == "flat")
        return;
    if (type == "pbr") {
        W::number(node, "roughness", "Rugosite GGX", .3, 0, 1);
        if (document_.data().at("render").string("integrator", "whitted") != "pbr")
            ImGui::TextWrapped("Activer le moteur pbr pour la rugosite GGX.");
    } else {
        W::number(node, "reflection", "Reflexion miroir", 0, 0, 1);
        W::number(node, "specular", "Brillance Phong", .25, 0, 1);
        W::number(node, "shininess", "Exposant", 64, 1, 2048);
    }
    W::number(node, "metallic", "Metal", 0, 0, 1);
    W::number(node, "transmission", "Transmission", 0, 0, 1);
    W::number(node, "ior", "Indice de refraction", 1.5, 1, 3);
    if (type == "pbr")
        W::vector(node, "absorption", "Absorption / unite de longueur", {});
    W::combo(node, "pattern", "Motif", {"solid", "checker", "marble", "perlin", "image"}, "solid");
    auto pattern = node.string("pattern", "solid");
    if (pattern != "solid") {
        W::number(node, "scale", "Frequence du motif", 1, .0001, 1000);
        if (pattern == "image") {
            W::text(node, "file", "Texture couleur");
            if (ImGui::Button("Parcourir texture"))
                chooseFile(StudioFileAction::Texture, "");
        } else
            W::color(node, "second", "Seconde couleur", {.1, .1, .1});
    }
    W::text(node, "normalFile", "Normal map");
    if (ImGui::Button("Parcourir normale"))
        chooseFile(StudioFileAction::Normal, "");
    if (node.has("normalFile") && node.at("normalFile").string().empty())
        std::get<Json::Object>(node.value).erase("normalFile");
    W::number(node, "normalStrength", "Intensite normale", 1, 0, 10);
    W::vector(node, "emission", "Emission lineaire", {});
}

void StudioApp::lightProperties(Json &node) {
    auto type = node.at("type").string();
    if (W::combo(node, "type", "Type", StudioPresets::lights(), type)) {
        node = StudioPresets::light(node.at("type").string());
        type = node.at("type").string();
    }
    W::color(node, "color", "Couleur", {1, 1, 1});
    W::number(node, "intensity", "Intensite", 1, 0,
              type == "point" || type == "area" ? 10000 : 100);
    if (type == "point" || type == "area")
        W::vector(node, "position", "Position", {0, 5, 0});
    if (type == "directional")
        W::vector(node, "direction", "Direction de propagation", {-1, -2, -1});
    if (type == "point") {
        W::flag(node, "physical", "Attenuation physique",
                document_.data().at("render").string("integrator", "whitted") != "whitted");
        W::number(node, "falloff", "Attenuation artistique", 0, 0, 100);
    }
    if (type == "area") {
        W::vector(node, "u", "Cote U", {2, 0, 0});
        W::vector(node, "v", "Cote V", {0, 0, 2});
        W::flag(node, "twoSided", "Emission sur les deux faces");
    }
}

void StudioApp::cameraProperties(Json &node) {
    W::vector(node, "position", "Position", {0, 2, 7});
    W::vector(node, "target", "Point vise", {0, 1, 0});
    W::vector(node, "up", "Verticale", {0, 1, 0});
    W::number(node, "fov", "Champ de vision", 45, 1, 175);
}

void StudioApp::renderProperties(Json &node) {
    W::combo(node, "integrator", "Moteur", {"pbr", "path", "whitted", "photon"}, "whitted");
    W::number(node, "width", "Largeur", 640, 1, 8192, true);
    W::number(node, "height", "Hauteur", 360, 1, 8192, true);
    W::number(node, "samples", "Echantillons par pixel", 16, 1, 4096, true);
    W::number(node, "depth", "Profondeur", 6, 1, 24, true);
    W::number(node, "threads", "Threads (0 = auto)", 0, 0, 256, true);
    W::number(node, "exposure", "Exposition", 1, .01, 100);
    W::combo(node, "toneMap", "Tone mapping", {"neutral", "reinhard", "none"}, "reinhard");
    W::combo(node, "colorSpace", "Encodage", {"srgb", "gamma2"},
             node.string("integrator", "whitted") == "pbr" ? "srgb" : "gamma2");
    W::number(node, "adaptive", "Erreur adaptative (0 = off)", 0, 0, 1);
    W::number(node, "minSamples", "Minimum adaptatif", 4, 2, 4096, true);
    W::number(node, "ao", "Occlusion ambiante", 0, 0, 128, true);
    if (node.string("integrator", "whitted") == "photon") {
        W::number(node, "photons", "Photons", 0, 0, 2000000, true);
        W::number(node, "photonRadius", "Rayon de collecte", .3, .001, 10);
    }
}

void StudioApp::environmentProperties() {
    auto &root = std::get<Json::Object>(document_.data().value);
    bool enabled = root.count("environment") != 0;
    if (ImGui::Checkbox("Activer une carte HDR", &enabled)) {
        if (enabled)
            root["environment"] = Json(
                Json::Object{{"file", Json((resourceRoot() / "bonus/assets/studio.hdr").string())},
                             {"intensity", Json(1.)}});
        else
            root.erase("environment");
    }
    if (enabled) {
        auto &node = root.at("environment");
        W::text(node, "file", "Fichier HDR / image");
        if (ImGui::Button("Parcourir environnement"))
            chooseFile(StudioFileAction::Environment, "");
        W::number(node, "intensity", "Intensite", 1, 0, 10000);
        W::number(node, "rotation", "Rotation (degres)", 0, -36000, 36000);
    }
    W::color(document_.data(), "background", "Fond uniforme", {.025, .035, .06});
}

void StudioApp::inspector(float width, float height) {
    ImGui::SetNextWindowPos({width - 330, 56});
    ImGui::SetNextWindowSize({330, height - 80});
    ImGui::Begin("Properties", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoSavedSettings);
    ImGui::SeparatorText("PROPRIETES");
    if (ImGui::IsKeyPressed(ImGuiKey_F2, false) &&
        !ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId))
        ImGui::SetKeyboardFocusHere();
    if (selection_ == StudioSelection::Material)
        ImGui::TextUnformatted(material_.c_str());
    if (selection_ == StudioSelection::Object || selection_ == StudioSelection::Light ||
        selection_ == StudioSelection::Material) {
        if (ImGui::Button("Dupliquer"))
            duplicate();
        ImGui::SameLine();
        if (ImGui::Button("Supprimer"))
            remove();
        ImGui::Separator();
    }
    W::beginEdits();
    auto before = document_.data().dump();
    ImGui::PushID(static_cast<int>(selection_));
    try {
        if (selection_ == StudioSelection::Environment)
            environmentProperties();
        else if (auto *node = selectedNode()) {
            if (selection_ == StudioSelection::Object)
                objectProperties(*node);
            if (selection_ == StudioSelection::Light)
                lightProperties(*node);
            if (selection_ == StudioSelection::Material)
                materialProperties(*node);
            if (selection_ == StudioSelection::Camera)
                cameraProperties(*node);
            if (selection_ == StudioSelection::Render)
                renderProperties(*node);
        }
    } catch (const std::exception &error) {
        message_ = error.what();
    }
    ImGui::PopID();
    if (document_.data().dump() != before) {
        // A drag or text entry is one undo step, even when it spans many frames.
        auto item = W::editedItem();
        if (item == 0 || item != propertyEdit_) {
            undo_.push_back(std::move(before));
            if (undo_.size() > 100)
                undo_.erase(undo_.begin());
        }
        propertyEdit_ = item;
        redo_.clear();
        edit();
    }
    if (!ImGui::IsAnyItemActive())
        propertyEdit_ = 0;
    ImGui::End();
}
} // namespace rt
#endif
