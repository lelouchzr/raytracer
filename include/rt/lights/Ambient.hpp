#pragma once
#include "rt/engine.hpp"

namespace rt {
class Ambient final : public ILight {
    Vec3 color_;

  public:
    explicit Ambient(Vec3 c);

    LightSample sample(Vec3) const override;
};
} // namespace rt
