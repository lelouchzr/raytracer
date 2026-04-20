#pragma once
#include "rt/math.hpp"

namespace rt {
struct Surface {
    Vec3 color{0.7, 0.7, 0.7}, emission{};
    double reflection = 0, transmission = 0, ior = 1.5, specular = 0, shininess = 64, metallic = 0;
};
} // namespace rt
