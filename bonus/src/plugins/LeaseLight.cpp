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

bool LeaseLight::emit(unsigned long long &rng, Ray &r, Vec3 &power) const {
    return light_->emit(rng, r, power);
}
} // namespace rt
