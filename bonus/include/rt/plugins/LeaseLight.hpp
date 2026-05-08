#pragma once
#include "rt/engine.hpp"
#include "rt/plugins/SharedLibrary.hpp"

namespace rt {
class LeaseLight final : public ILight {
    std::shared_ptr<SharedLibrary> library_;
    std::shared_ptr<const ILight> light_;

  public:
    LeaseLight(std::shared_ptr<SharedLibrary>, std::shared_ptr<const ILight>);
    LightSample sample(Vec3) const override;
    LightSample sample(Vec3, unsigned long long &) const override;
    bool intersect(const Ray &, double, double &, Vec3 &) const override;
    double directionPdf(Vec3, Vec3) const override;
    Vec3 environment(Vec3) const override;
    bool emit(unsigned long long &, Ray &, Vec3 &) const override;
};
} // namespace rt
