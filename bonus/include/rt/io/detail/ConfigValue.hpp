#pragma once

#include <string>
#include <utility>
#include <vector>

namespace rt::io {

enum class ConfigKind { Object, Array, String, Number, Boolean };

struct ConfigValue {
    ConfigKind kind = ConfigKind::Object;
    double number = 0;
    std::string text;
    // Ordered members preserve repeated historical primitive blocks.
    std::vector<std::pair<std::string, ConfigValue>> members;
    std::vector<ConfigValue> elements;
};

} // namespace rt::io
