#pragma once
#include "rt/engine.hpp"

namespace rt {
class BasicIntegrator final : public IIntegrator {
  public:
    Vec3 trace(const Scene &s, Ray ray, int, unsigned long long &) const override;
};
} // namespace rt
