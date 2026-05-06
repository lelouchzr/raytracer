#include "rt/engine.hpp"

namespace rt {
ILight::~ILight() = default;

LightSample ILight::sample(Vec3 point, unsigned long long &) const {
    return sample(point);
}

bool ILight::intersect(const Ray &, double, double &, Vec3 &) const {
    return false;
}
} // namespace rt

namespace rt {
bool ILight::emit(unsigned long long &, Ray &, Vec3 &) const {
    return false;
}
} // namespace rt
