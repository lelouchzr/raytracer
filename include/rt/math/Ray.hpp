#pragma once
#include "rt/math/Constants.hpp"
#include "rt/math/Vec3.hpp"

namespace rt {
struct Ray {
    Vec3 origin, direction;
    Vec3 at(double t) const;
};
} // namespace rt
