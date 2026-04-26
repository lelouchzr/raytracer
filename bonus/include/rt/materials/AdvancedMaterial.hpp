#pragma once
#include "rt/engine.hpp"
#include "rt/io/Image.hpp"
#include "rt/materials/Perlin.hpp"

namespace rt {
class AdvancedMaterial final : public IMaterial {
    Surface surface_;
    Vec3 second_;
    std::string pattern_;
    double scale_, normalStrength_;
    Perlin perlin_;
    std::shared_ptr<const Image> texture_, normalMap_;

  public:
    AdvancedMaterial(const Json &, const std::filesystem::path &);
    Surface sample(Vec3) const override;
    Surface sampleHit(const Hit &) const override;
    Vec3 shadingNormal(const Hit &) const override;
};
} // namespace rt
