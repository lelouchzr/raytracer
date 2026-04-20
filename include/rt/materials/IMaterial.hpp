#pragma once
#include "rt/materials/Surface.hpp"

namespace rt {
struct Hit;

class IMaterial {
  public:
    virtual ~IMaterial();
    virtual Surface sample(Vec3 local) const = 0;
    virtual Surface sampleHit(const Hit &) const;
    virtual Vec3 shadingNormal(const Hit &) const;
};
} // namespace rt
