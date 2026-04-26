#pragma once
#include "rt/engine.hpp"

namespace rt {
class Composite final : public IPrimitive {
    std::vector<Primitive> children_;
    Bounds bounds_;
    bool bounded_ = true;

  public:
    explicit Composite(std::vector<Primitive> children);

    bool intersect(const Ray &r, double lo, double hi, Hit &h) const override;

    Bounds bounds() const override;
};
} // namespace rt
