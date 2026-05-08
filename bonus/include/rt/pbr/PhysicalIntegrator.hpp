#pragma once
#include "rt/engine.hpp"
#include "rt/render/MediumStack.hpp"

namespace rt {
class PhysicalIntegrator final : public IIntegrator {
    MediumStack initial_;
    Vec3 origin_;

  public:
    explicit PhysicalIntegrator(const Scene &);
    Vec3 trace(const Scene &, Ray, int, unsigned long long &) const override;
    static double powerWeight(double, double);
};
} // namespace rt
