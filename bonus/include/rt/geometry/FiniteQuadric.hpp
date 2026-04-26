#pragma once
#include "rt/engine.hpp"

namespace rt {
class FiniteQuadric final : public Shape {
    double radius_, height_;
    bool cone_;

  public:
    FiniteQuadric(double r, double height, bool cone, std::shared_ptr<const IMaterial> m);

    bool intersect(const Ray &r, double lo, double hi, Hit &h) const override;

    Bounds bounds() const override;
};
} // namespace rt
