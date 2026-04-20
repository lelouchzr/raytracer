#pragma once
#include "rt/math.hpp"

namespace rt {
struct LightSample {
    Vec3 direction, radiance;
    double distance = infinity;
    bool ambient = false;
};
} // namespace rt
