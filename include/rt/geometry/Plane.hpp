#pragma once
#include "rt/engine.hpp"

namespace rt {
class Plane final : public Shape {
    Vec3 normal_;

  public:
    Plane(Vec3 n, std::shared_ptr<const IMaterial> m);

    bool intersect(const Ray &r, double lo, double hi, Hit &h) const override;

    Bounds bounds() const override;
};
} // namespace rt
