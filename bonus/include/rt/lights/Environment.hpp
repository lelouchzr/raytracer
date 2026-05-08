#pragma once
#include "rt/engine.hpp"
#include "rt/io/Image.hpp"

namespace rt {
class Environment final : public ILight {
    Image image_;
    double intensity_, rotation_, total_ = 0;
    std::vector<double> cdf_, probabilities_;
    std::pair<int, int> pixel(Vec3) const;

  public:
    Environment(const Json &, const std::filesystem::path &);
    LightSample sample(Vec3) const override;
    LightSample sample(Vec3, unsigned long long &) const override;
    double directionPdf(Vec3, Vec3) const override;
    Vec3 environment(Vec3) const override;
};
} // namespace rt
