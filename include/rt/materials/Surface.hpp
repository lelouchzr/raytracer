#pragma once
#include "rt/math.hpp"

namespace rt {
struct Surface {
    Vec3 color{0.7, 0.7, 0.7}, emission{};
    bool physical = false;
    double roughness = .3;
    Vec3 absorption{};
    double reflection = 0, transmission = 0, ior = 1.5, specular = 0, shininess = 64, metallic = 0;
};
} // namespace rt
