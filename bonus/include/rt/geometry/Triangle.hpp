#pragma once
#include "rt/engine.hpp"

namespace rt {
class Triangle final : public Shape {
    std::array<Vec3, 3> vertices_, normals_, uvs_;
    Vec3 normal_;
    bool smooth_ = false, textured_ = false;

  public:
    Triangle(Vec3, Vec3, Vec3, std::shared_ptr<const IMaterial>);
    Triangle(std::array<Vec3, 3>, std::array<Vec3, 3>, std::array<Vec3, 3>, bool, bool,
             std::shared_ptr<const IMaterial>);
    bool intersect(const Ray &, double, double, Hit &) const override;
    Bounds bounds() const override;
};
} // namespace rt
