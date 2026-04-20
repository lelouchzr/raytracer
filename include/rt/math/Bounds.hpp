#pragma once
#include "rt/math/Constants.hpp"
#include "rt/math/Ray.hpp"

namespace rt {
struct Bounds {
    Vec3 min{infinity, infinity, infinity}, max{-infinity, -infinity, -infinity};
    void include(Vec3 p);

    void include(Bounds b);

    bool finite() const;

    bool hit(Ray r, double lo, double hi) const;
};
} // namespace rt
