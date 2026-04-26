#pragma once
#include "rt/engine.hpp"
#include "rt/render/Photon.hpp"

namespace rt {
class RenderControl;

class PhotonMap {
    std::vector<Photon> photons_;
    double radius_;
    std::map<std::array<long long, 3>, std::vector<std::size_t>> grid_;
    std::array<long long, 3> cell(Vec3) const;

  public:
    explicit PhotonMap(const Scene &, const RenderControl *control = nullptr);
    Vec3 radiance(Vec3 point, Vec3 normal, Vec3 albedo, const void *receiver = nullptr) const;
    std::size_t countFor(const void *) const;
    std::size_t size() const;
};
} // namespace rt
