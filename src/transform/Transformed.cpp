#include "rt/engine.hpp"

namespace rt {
Transformed::Transformed(Primitive child, Transform t) : child_(std::move(child)), transform_(t) {}
} // namespace rt

namespace rt {
bool Transformed::intersect(const Ray &r, double lo, double hi, Hit &h) const {
    if (!child_->intersect({transform_.local(r.origin), transform_.rotate(r.direction, true)}, lo,
                           hi, h))
        return false;
    h.point = r.at(h.t);
    h.normal = transform_.rotate(h.normal);
    h.geometric = transform_.rotate(h.geometric);
    h.tangent = transform_.rotate(h.tangent);
    h.bitangent = transform_.rotate(h.bitangent);
    h.boundary = this;
    return true;
}

Bounds Transformed::bounds() const {
    Bounds b = child_->bounds();
    if (!b.finite())
        return b;
    Bounds out;
    for (int i = 0; i < 8; ++i)
        out.include(transform_.point({(i & 1) ? b.max.x : b.min.x, (i & 2) ? b.max.y : b.min.y,
                                      (i & 4) ? b.max.z : b.min.z}));
    return out;
}

} // namespace rt
