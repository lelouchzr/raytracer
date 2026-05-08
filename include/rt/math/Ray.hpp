#pragma once
#include "rt/math/Constants.hpp"
#include "rt/math/Vec3.hpp"

namespace rt {
struct Ray {
    Vec3 origin, direction;
    double coneWidth = 0, coneSpread = 0;
    Vec3 at(double t) const;
};
} // namespace rt
