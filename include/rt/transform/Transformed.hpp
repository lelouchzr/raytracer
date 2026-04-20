#pragma once
#include "rt/geometry/IPrimitive.hpp"

namespace rt {
class Transformed final : public IPrimitive {
    Primitive child_;
    Transform transform_;

  public:
    Transformed(Primitive child, Transform t);

    bool intersect(const Ray &, double, double, Hit &) const override;
    Bounds bounds() const override;
};
} // namespace rt
