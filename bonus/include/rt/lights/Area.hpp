#pragma once
#include "rt/engine.hpp"

namespace rt {
class Area final : public ILight {
    Vec3 center_, u_, v_, normal_, radiance_;
    double area_, uu_, uv_, vv_;
    bool twoSided_ = false;
    LightSample towards(Vec3, Vec3) const;

  public:
    explicit Area(const Json &);
    LightSample sample(Vec3) const override;
    LightSample sample(Vec3, unsigned long long &) const override;
    bool intersect(const Ray &, double, double &, Vec3 &) const override;
    double directionPdf(Vec3, Vec3) const override;
    bool emit(unsigned long long &, Ray &, Vec3 &) const override;
};
} // namespace rt
