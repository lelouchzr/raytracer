#include "rt/engine.hpp"

namespace rt {
ILight::~ILight() = default;
}

namespace rt {
bool ILight::emit(unsigned long long &, Ray &, Vec3 &) const {
    return false;
}
} // namespace rt
