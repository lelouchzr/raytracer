#pragma once
#include "rt/math.hpp"

namespace rt {
struct LightSample {
    Vec3 direction, radiance;
    double distance = infinity;
    bool ambient = false;
    double pdf = 0;
};
} // namespace rt
