#pragma once
#include "rt/geometry/Hit.hpp"
#include <vector>

namespace rt {
struct Scene;

class MediumStack {
    std::vector<std::pair<const void *, double>> media_;

  public:
    static MediumStack atOrigin(const Scene &, Vec3);
    double current() const;
    MediumStack cross(const Hit &, double ior) const;
    std::pair<double, double> indices(const Hit &, double ior) const;
};
} // namespace rt
