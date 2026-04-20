#pragma once
#include "rt/materials/IMaterial.hpp"

namespace rt {
struct Hit {
    double t = infinity;
    Vec3 point, local, normal, geometric, uv, tangent, bitangent;
    bool hasUV = false;
    const void *boundary = nullptr;
    bool front = true;
    const IMaterial *material = nullptr;
};
} // namespace rt
