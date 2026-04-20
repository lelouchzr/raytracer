#pragma once
#include "rt/math.hpp"

namespace rt {
struct Scene;

class IIntegrator {
  public:
    virtual ~IIntegrator();
    virtual Vec3 trace(const Scene &, Ray, int depth, unsigned long long &rng) const = 0;
};
} // namespace rt
