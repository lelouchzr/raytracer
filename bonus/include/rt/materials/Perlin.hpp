#pragma once
#include "rt/math.hpp"

namespace rt {
class Perlin {
    std::array<int, 512> permutation_;
    static double gradient(int, double, double, double);

  public:
    Perlin();
    double noise(Vec3) const;
};
} // namespace rt
