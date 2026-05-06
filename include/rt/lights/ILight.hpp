#pragma once
#include "rt/lights/LightSample.hpp"

namespace rt {
class ILight {
  public:
    virtual ~ILight();
    virtual LightSample sample(Vec3) const = 0;
    virtual LightSample sample(Vec3, unsigned long long &) const;
    virtual bool intersect(const Ray &, double, double &, Vec3 &) const;
    virtual bool emit(unsigned long long &, Ray &, Vec3 &) const;
};
} // namespace rt
