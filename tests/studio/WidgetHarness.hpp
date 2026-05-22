#pragma once
#include "rt/json.hpp"
#include <array>
#include <string>

struct ImGuiContext;

namespace rt::test {
enum class WidgetKind { Vector, Number };

class WidgetHarness {
    ImGuiContext *context_;
    Json &node_;
    std::string key_;
    WidgetKind kind_;
    std::array<float, 4> bounds_{};
    void frame();

  public:
    WidgetHarness(Json &, std::string, WidgetKind = WidgetKind::Vector);
    ~WidgetHarness();
    WidgetHarness(const WidgetHarness &) = delete;
    WidgetHarness &operator=(const WidgetHarness &) = delete;
    void type(const std::string &, int component = 0);
};
} // namespace rt::test
