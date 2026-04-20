#pragma once
#include "rt/engine.hpp"

namespace rt {
class Directional final : public ILight {
    Vec3 direction_, color_;

  public:
    Directional(Vec3 d, Vec3 c);

    LightSample sample(Vec3) const override;
};
} // namespace rt
