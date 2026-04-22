#include "rt/lights/Ambient.hpp"

namespace rt {
Ambient::Ambient(Vec3 c) : color_(c) {}

LightSample Ambient::sample(Vec3) const {
    return {{}, color_, infinity, true};
}
} // namespace rt
