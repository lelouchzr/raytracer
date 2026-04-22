#include "rt/engine.hpp"

namespace rt {
FlatMaterial::FlatMaterial(Vec3 c) : color_(c) {}

Surface FlatMaterial::sample(Vec3) const {
    Surface s;
    s.color = color_;
    return s;
}
} // namespace rt
