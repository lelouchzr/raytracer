#pragma once
#include "rt/engine.hpp"

namespace rt {
class Implicit final : public Shape {
    std::string type_;
    double major_, minor_;
    int iterations_;
    double field(Vec3) const;

  public:
    Implicit(const Json &, std::shared_ptr<const IMaterial>);
    bool intersect(const Ray &, double, double, Hit &) const override;
    Bounds bounds() const override;
};

void registerImplicit(Factory &);
} // namespace rt
