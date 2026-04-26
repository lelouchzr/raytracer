#pragma once
#include "rt/engine.hpp"

namespace rt {
class Scaled final : public IPrimitive {
    Primitive child_;
    Vec3 scale_;

  public:
    Scaled(Primitive p, Vec3 scale);

    bool intersect(const Ray &r, double lo, double hi, Hit &h) const override;

    Bounds bounds() const override;
};
} // namespace rt
