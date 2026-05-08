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
double ILight::directionPdf(Vec3, Vec3) const {
    return 0;
}

Vec3 ILight::environment(Vec3) const {
    return {};
}

bool ILight::emit(unsigned long long &, Ray &, Vec3 &) const {
    return false;
}
} // namespace rt
