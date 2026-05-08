#pragma once
#include "rt/materials/IMaterial.hpp"

namespace rt {
struct Hit {
    double t = infinity;
    Vec3 point, local, normal, geometric, uv, tangent, bitangent;
    double footprint = 0, coneSpread = 0, textureScale = 1;
    bool hasUV = false;
    const void *boundary = nullptr;
    bool front = true;
    const IMaterial *material = nullptr;
};
} // namespace rt
