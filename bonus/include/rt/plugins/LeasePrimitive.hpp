#pragma once
#include "rt/engine.hpp"
#include "rt/plugins/SharedLibrary.hpp"

namespace rt {
class LeasePrimitive final : public IPrimitive {
    std::shared_ptr<SharedLibrary> library_;
    Primitive primitive_;

  public:
    LeasePrimitive(std::shared_ptr<SharedLibrary>, Primitive);
    bool intersect(const Ray &, double, double, Hit &) const override;
    Bounds bounds() const override;
};
} // namespace rt
