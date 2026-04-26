#pragma once
#include "rt/geometry/IPrimitive.hpp"
#include "rt/math/Mat4.hpp"

namespace rt {
class AffineTransform final : public IPrimitive {
    Primitive child_;
    Mat4 matrix_, inverse_;
    bool identifyBoundary_;

  public:
    AffineTransform(Primitive, const Mat4 &, bool identifyBoundary = true);
    bool intersect(const Ray &, double, double, Hit &) const override;
    Bounds bounds() const override;
};

Primitive applyAffine(Primitive, const Json &);
} // namespace rt
