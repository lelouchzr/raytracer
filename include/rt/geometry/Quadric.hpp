#pragma once
#include "rt/engine.hpp"

namespace rt {
class Quadric final : public Shape {
    double radius_;
    bool cone_;

  public:
    Quadric(double r, bool cone, std::shared_ptr<const IMaterial> m);

    bool intersect(const Ray &r, double lo, double hi, Hit &h) const override;

    Bounds bounds() const override;
};
} // namespace rt
