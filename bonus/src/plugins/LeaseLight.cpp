#include "rt/plugins/LeaseLight.hpp"

namespace rt {
LeaseLight::LeaseLight(std::shared_ptr<SharedLibrary> library, std::shared_ptr<const ILight> light)
    : library_(std::move(library)), light_(std::move(light)) {
    if (!light_)
        throw std::runtime_error("plugin returned a null light");
}

LightSample LeaseLight::sample(Vec3 p) const {
    return light_->sample(p);
}

LightSample LeaseLight::sample(Vec3 p, unsigned long long &rng) const {
    return light_->sample(p, rng);
}

bool LeaseLight::intersect(const Ray &ray, double lo, double &hi, Vec3 &emission) const {
    return light_->intersect(ray, lo, hi, emission);
}

double LeaseLight::directionPdf(Vec3 p, Vec3 d) const {
    return light_->directionPdf(p, d);
}

Vec3 LeaseLight::environment(Vec3 d) const {
    return light_->environment(d);
}

bool LeaseLight::emit(unsigned long long &rng, Ray &r, Vec3 &power) const {
    return light_->emit(rng, r, power);
}
} // namespace rt
