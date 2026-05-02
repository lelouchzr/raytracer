#pragma once
#include "rt/engine.hpp"

namespace rt {
class PluginSphere final : public Shape {
    double radius_;

  public:
    PluginSphere(double, std::shared_ptr<const IMaterial>);
    bool intersect(const Ray &, double, double, Hit &) const override;
    Bounds bounds() const override;
};
} // namespace rt
