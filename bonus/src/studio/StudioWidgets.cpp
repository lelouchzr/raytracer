#include "rt/studio/StudioWidgets.hpp"
#include "rt/math/Color.hpp"
#ifdef RT_NATIVE_STUDIO
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>
#endif

namespace rt {
Json StudioWidgets::vec(Vec3 v) {
    return Json(Json::Array{Json(v.x), Json(v.y), Json(v.z)});
}

Json &StudioWidgets::member(Json &json, const std::string &key, Json fallback) {
    auto &object = std::get<Json::Object>(json.value);
    return object.try_emplace(key, std::move(fallback)).first->second;
}

#ifdef RT_NATIVE_STUDIO
bool StudioWidgets::number(Json &json, const char *key, const char *label, double fallback,
                           double lo, double hi, bool integer) {
    double value = json.number(key, fallback);
    ImGui::TextUnformatted(label);
    ImGui::SetNextItemWidth(-1);
    if (!ImGui::DragScalar((std::string("##") + key).c_str(), ImGuiDataType_Double, &value,
                           integer ? 1.f : .025f, &lo, &hi, integer ? "%.0f" : "%.3f",
                           ImGuiSliderFlags_AlwaysClamp))
        return false;
    if (!std::isfinite(value))
        throw std::runtime_error("non-finite studio value");
    if (integer)
        value = std::round(value);
    member(json, key, Json(value)) = Json(value);
    return true;
}

bool StudioWidgets::vector(Json &json, const char *key, const char *label, Vec3 fallback) {
    Vec3 v = json.vector(key, fallback);
    double values[3]{v.x, v.y, v.z};
    ImGui::TextUnformatted(label);
    ImGui::SetNextItemWidth(-1);
    if (!ImGui::DragScalarN((std::string("##") + key).c_str(), ImGuiDataType_Double, values, 3,
                            .025f, nullptr, nullptr, "%.3f"))
        return false;
    for (double value : values)
        if (!std::isfinite(value) || std::abs(value) > 1e9)
            throw std::runtime_error("studio vector outside finite range");
    member(json, key, vec({})) = vec({values[0], values[1], values[2]});
    return true;
}

bool StudioWidgets::color(Json &json, const char *key, const char *label, Vec3 fallback) {
    auto v = json.vector(key, fallback);
    float color[3]{static_cast<float>(srgbEncode(v.x)), static_cast<float>(srgbEncode(v.y)),
                   static_cast<float>(srgbEncode(v.z))};
    ImGui::TextUnformatted(label);
    ImGui::SetNextItemWidth(-1);
    if (!ImGui::ColorEdit3((std::string("##") + key).c_str(), color, ImGuiColorEditFlags_Float))
        return false;
    member(json, key, vec({})) =
        vec({srgbDecode(color[0]), srgbDecode(color[1]), srgbDecode(color[2])});
    return true;
}

bool StudioWidgets::text(Json &json, const char *key, const char *label,
                         const std::string &fallback) {
    auto value = json.string(key, fallback);
    ImGui::TextUnformatted(label);
    ImGui::SetNextItemWidth(-1);
    if (!ImGui::InputText((std::string("##") + key).c_str(), &value))
        return false;
    member(json, key, Json(value)) = Json(value);
    return true;
}

bool StudioWidgets::flag(Json &json, const char *key, const char *label, bool fallback) {
    bool value = json.has(key) ? std::get<bool>(json.at(key).value) : fallback;
    if (!ImGui::Checkbox(label, &value))
        return false;
    member(json, key, Json(value)) = Json(value);
    return true;
}

bool StudioWidgets::combo(Json &json, const char *key, const char *label,
                          const std::vector<std::string> &items, const std::string &fallback) {
    auto value = json.string(key, fallback);
    bool changed = false;
    ImGui::TextUnformatted(label);
    ImGui::SetNextItemWidth(-1);
    if (ImGui::BeginCombo((std::string("##") + key).c_str(), value.c_str())) {
        for (const auto &item : items)
            if (ImGui::Selectable(item.c_str(), item == value)) {
                member(json, key, Json(item)) = Json(item);
                changed = true;
            }
        ImGui::EndCombo();
    }
    return changed;
}
#endif
} // namespace rt
