#pragma once
#include "rt/engine.hpp"

namespace rt {
class Box final : public Shape {
    Vec3 half_;

  public:
    Box(Vec3 half, std::shared_ptr<const IMaterial> m);

    bool intersect(const Ray &r, double lo, double hi, Hit &h) const override;

    Bounds bounds() const override;
};
} // namespace rt
