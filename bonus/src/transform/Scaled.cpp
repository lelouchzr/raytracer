#include "rt/transform/Scaled.hpp"
#include "rt/Extensions.hpp"

namespace rt {
Scaled::Scaled(Primitive p, Vec3 scale) : child_(std::move(p)), scale_(scale) {}

bool Scaled::intersect(const Ray &r, double lo, double hi, Hit &h) const {
    auto divide = [&](Vec3 v) { return Vec3{v.x / scale_.x, v.y / scale_.y, v.z / scale_.z}; };
    if (!child_->intersect({divide(r.origin), divide(r.direction)}, lo, hi, h))
        return false;
    h.point = r.at(h.t);
    h.normal = unit(divide(h.normal));
    h.geometric = unit(divide(h.geometric));
    h.tangent = unit(h.tangent * scale_);
    h.bitangent = unit(h.bitangent * scale_);
    h.boundary = this;
    return true;
}

Bounds Scaled::bounds() const {
    Bounds b = child_->bounds();
    if (!b.finite())
        return b;
    Bounds result;
    result.include(b.min * scale_);
    result.include(b.max * scale_);
    return result;
}
} // namespace rt

namespace rt {
Primitive applyScale(Primitive p, Vec3 scale) {
    for (int i = 0; i < 3; ++i)
        range(std::abs(scale[i]), 1e-4, 1e4, "absolute object scale");
    return std::make_shared<Scaled>(std::move(p), scale);
}

} // namespace rt
