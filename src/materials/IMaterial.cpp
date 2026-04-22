#include "rt/engine.hpp"

namespace rt {
IMaterial::~IMaterial() = default;
}

namespace rt {
Surface IMaterial::sampleHit(const Hit &h) const {
    return sample(h.local);
}

Vec3 IMaterial::shadingNormal(const Hit &h) const {
    return h.normal;
}
} // namespace rt
