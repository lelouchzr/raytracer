#pragma once
#include "rt/geometry/Hit.hpp"
#include <memory>

namespace rt {
class IPrimitive {
  public:
    virtual ~IPrimitive();
    virtual bool intersect(const Ray &, double lo, double hi, Hit &) const = 0;
    virtual Bounds bounds() const = 0;
};

using Primitive = std::shared_ptr<const IPrimitive>;
} // namespace rt
