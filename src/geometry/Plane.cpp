#include "rt/geometry/Plane.hpp"

namespace rt {
Plane::Plane(Vec3 n, std::shared_ptr<const IMaterial> m) : Shape(std::move(m)), normal_(unit(n)) {}

bool Plane::intersect(const Ray &r, double lo, double hi, Hit &h) const {
    double d = dot(normal_, r.direction);
    return std::abs(d) > 1e-12 && finish(r, -dot(normal_, r.origin) / d, normal_, lo, hi, h);
}

Bounds Plane::bounds() const {
    return {{-infinity, -infinity, -infinity}, {infinity, infinity, infinity}};
}
} // namespace rt
