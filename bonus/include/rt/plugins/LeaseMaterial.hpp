#pragma once
#include "rt/engine.hpp"
#include "rt/plugins/SharedLibrary.hpp"

namespace rt {
class LeaseMaterial final : public IMaterial {
    std::shared_ptr<SharedLibrary> library_;
    std::shared_ptr<const IMaterial> material_;

  public:
    LeaseMaterial(std::shared_ptr<SharedLibrary>, std::shared_ptr<const IMaterial>);
    Surface sample(Vec3) const override;
    Surface sampleHit(const Hit &) const override;
    Vec3 shadingNormal(const Hit &) const override;
};
} // namespace rt
