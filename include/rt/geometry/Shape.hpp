#pragma once
#include "rt/geometry/IPrimitive.hpp"

namespace rt {
class Shape : public IPrimitive {
  protected:
    std::shared_ptr<const IMaterial> material_;
    bool finish(Ray r, double t, Vec3 normal, double lo, double hi, Hit &h) const;

  public:
    explicit Shape(std::shared_ptr<const IMaterial> m);
};
} // namespace rt
