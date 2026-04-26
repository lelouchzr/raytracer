#pragma once
#include "rt/engine.hpp"
#include "rt/render/MediumStack.hpp"
#include "rt/render/PhotonMap.hpp"

namespace rt {
class AdvancedIntegrator final : public IIntegrator {
    std::shared_ptr<const PhotonMap> photons_;
    MediumStack initial_;
    Vec3 origin_{infinity, infinity, infinity};
    Vec3 radiance(const Scene &, Ray, int, unsigned long long &, const MediumStack &, bool) const;

  public:
    explicit AdvancedIntegrator(std::shared_ptr<const PhotonMap> photons = {});
    AdvancedIntegrator(const Scene &, std::shared_ptr<const PhotonMap>);
    Vec3 trace(const Scene &, Ray, int, unsigned long long &) const override;
};
} // namespace rt
