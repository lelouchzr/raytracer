#pragma once

#include "rt/json.hpp"
#include <string>

namespace rt {

// JSON passes through unchanged. Historical and libconfig scenes are normalized
// before the regular scene builder validates geometry, materials and settings.
Json adaptScene(const std::string &text);

} // namespace rt
