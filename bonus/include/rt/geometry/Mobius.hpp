#pragma once
#include "rt/engine.hpp"

namespace rt {
class Mobius final : public IPrimitive {
    Primitive mesh_;

  public:
    Mobius(const Json &, std::shared_ptr<const IMaterial>);
    bool intersect(const Ray &, double, double, Hit &) const override;
    Bounds bounds() const override;
};

void registerMobius(Factory &);
} // namespace rt
