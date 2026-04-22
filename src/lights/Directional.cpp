#include "rt/lights/Directional.hpp"

namespace rt {
Directional::Directional(Vec3 d, Vec3 c) : direction_(unit(-d)), color_(c) {}

LightSample Directional::sample(Vec3) const {
    return {direction_, color_, infinity, false};
}
} // namespace rt
