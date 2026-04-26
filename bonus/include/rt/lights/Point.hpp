#pragma once
#include "rt/engine.hpp"

namespace rt {
class Point final : public ILight {
    Vec3 position_, radiance_;
    double falloff_;
    bool physical_;

  public:
    explicit Point(const Json &);
    LightSample sample(Vec3) const override;
    bool emit(unsigned long long &, Ray &, Vec3 &) const override;
};
} // namespace rt
