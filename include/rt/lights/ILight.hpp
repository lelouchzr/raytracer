#pragma once
#include "rt/lights/LightSample.hpp"

namespace rt {
class ILight {
  public:
    virtual ~ILight();
    virtual LightSample sample(Vec3) const = 0;
    virtual bool emit(unsigned long long &, Ray &, Vec3 &) const;
};
} // namespace rt
