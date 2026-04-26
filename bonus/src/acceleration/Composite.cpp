#include "rt/acceleration/Composite.hpp"
#include "rt/Extensions.hpp"

namespace rt {
Composite::Composite(std::vector<Primitive> children) : children_(std::move(children)) {
    for (const auto &c : children_) {
        if (!c->bounds().finite())
            bounded_ = false;
        else
            bounds_.include(c->bounds());
    }
}

bool Composite::intersect(const Ray &r, double lo, double hi, Hit &h) const {
    if (bounded_ && !bounds_.hit(r, lo, hi))
        return false;
    bool found = false;
    for (const auto &c : children_)
        if (c->intersect(r, lo, hi, h)) {
            hi = h.t;
            found = true;
        }
    return found;
}

Bounds Composite::bounds() const {
    return bounded_ ? bounds_
                    : Bounds{{-infinity, -infinity, -infinity}, {infinity, infinity, infinity}};
}
} // namespace rt
