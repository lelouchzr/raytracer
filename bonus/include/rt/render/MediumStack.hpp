#pragma once
#include "rt/geometry/Hit.hpp"
#include <vector>

namespace rt {
struct Scene;

struct Medium {
    const void *boundary;
    double ior;
    Vec3 absorption;
};

class MediumStack {
    std::vector<Medium> media_;

  public:
    static MediumStack atOrigin(const Scene &, Vec3);
    double current() const;
    MediumStack cross(const Hit &, double ior, Vec3 absorption = {}) const;
    Vec3 transmittance(double distance) const;
    std::pair<double, double> indices(const Hit &, double ior) const;
};
} // namespace rt
