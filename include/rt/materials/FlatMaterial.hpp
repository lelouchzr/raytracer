#pragma once
#include "rt/materials/IMaterial.hpp"

namespace rt {
class FlatMaterial final : public IMaterial {
    Vec3 color_;

  public:
    explicit FlatMaterial(Vec3 c);

    Surface sample(Vec3) const override;
};
} // namespace rt
