#include "WidgetHarness.hpp"
#include "rt/studio/StudioWidgets.hpp"
#include <imgui.h>
#include <utility>

namespace rt::test {
WidgetHarness::WidgetHarness(Json &node, std::string key, WidgetKind kind)
    : context_(ImGui::CreateContext()), node_(node), key_(std::move(key)), kind_(kind) {
    auto &io = ImGui::GetIO();
    io.DisplaySize = {640, 480};
    io.DeltaTime = 1.f / 60;
    io.IniFilename = nullptr;
    io.ConfigMacOSXBehaviors = false;
    io.AddFocusEvent(true);
    StudioWidgets::configureInputs();
    unsigned char *pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
    frame();
    frame();
}

WidgetHarness::~WidgetHarness() {
    ImGui::DestroyContext(context_);
}

void WidgetHarness::frame() {
    ImGui::SetCurrentContext(context_);
    ImGui::NewFrame();
    ImGui::SetNextWindowPos({0, 0});
    ImGui::SetNextWindowSize({640, 480});
    ImGui::Begin("Widget test", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoSavedSettings);
    StudioWidgets::beginEdits();
    ImGui::SetNextItemWidth(300);
    if (kind_ == WidgetKind::Vector)
        StudioWidgets::vector(node_, key_.c_str(), key_.c_str(), {});
    else
        StudioWidgets::number(node_, key_.c_str(), key_.c_str(), 0, -100, 100);
    auto lo = ImGui::GetItemRectMin(), hi = ImGui::GetItemRectMax();
    bounds_ = {lo.x, lo.y, hi.x, hi.y};
    ImGui::End();
    ImGui::Render();
}

void WidgetHarness::type(const std::string &text, int component) {
    auto &io = ImGui::GetIO();
    const float columns = kind_ == WidgetKind::Vector ? 3.f : 1.f;
    const float x =
        bounds_[0] + (bounds_[2] - bounds_[0]) * (static_cast<float>(component) + .5f) / columns;
    io.AddMousePosEvent(x, (bounds_[1] + bounds_[3]) * .5f);
    frame();
    io.AddMouseButtonEvent(0, true);
    frame();
    io.AddMouseButtonEvent(0, false);
    frame();
    frame();
    io.AddKeyEvent(ImGuiMod_Ctrl, true);
    io.AddKeyEvent(ImGuiKey_A, true);
    frame();
    io.AddKeyEvent(ImGuiKey_A, false);
    io.AddKeyEvent(ImGuiMod_Ctrl, false);
    frame();
    io.AddInputCharactersUTF8(text.c_str());
    frame();
    io.AddKeyEvent(ImGuiKey_Enter, true);
    frame();
    io.AddKeyEvent(ImGuiKey_Enter, false);
    frame();
}
} // namespace rt::test
