#include "rt/plugins/LeaseMaterial.hpp"

namespace rt {
LeaseMaterial::LeaseMaterial(std::shared_ptr<SharedLibrary> library,
                             std::shared_ptr<const IMaterial> material)
    : library_(std::move(library)), material_(std::move(material)) {
    if (!material_)
        throw std::runtime_error("plugin returned a null material");
}

Surface LeaseMaterial::sample(Vec3 p) const {
    return material_->sample(p);
}

Surface LeaseMaterial::sampleHit(const Hit &h) const {
    return material_->sampleHit(h);
}

Vec3 LeaseMaterial::shadingNormal(const Hit &h) const {
    return material_->shadingNormal(h);
}
} // namespace rt
