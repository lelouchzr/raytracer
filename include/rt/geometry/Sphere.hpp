#pragma once
#include "rt/engine.hpp"

namespace rt {
class Sphere final : public Shape {
    double radius_;

  public:
    Sphere(double r, std::shared_ptr<const IMaterial> m);

    bool intersect(const Ray &r, double lo, double hi, Hit &h) const override;

    Bounds bounds() const override;
};
} // namespace rt
