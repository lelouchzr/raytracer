#include "rt/engine.hpp"

namespace rt {
Shape::Shape(std::shared_ptr<const IMaterial> m) : material_(std::move(m)) {}
} // namespace rt

namespace rt {
bool Shape::finish(Ray r, double t, Vec3 normal, double lo, double hi, Hit &h) const {
    if (!std::isfinite(t) || t < lo || t > hi)
        return false;
    h.t = t;
    h.point = h.local = r.at(t);
    h.front = dot(r.direction, normal) < 0;
    h.normal = h.front ? unit(normal) : -unit(normal);
    h.geometric = h.normal;
    h.hasUV = false;
    h.textureScale = 1;
    h.boundary = this;
    h.tangent = unit(cross(std::abs(h.normal.y) < .9 ? Vec3{0, 1, 0} : Vec3{1, 0, 0}, h.normal));
    h.bitangent = cross(h.normal, h.tangent);
    h.material = material_.get();
    return true;
}

namespace {

// Infinite quadrics; optional finite capped forms are provided by the bonus registry.

} // namespace

} // namespace rt
