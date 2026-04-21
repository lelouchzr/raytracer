#include "rt/geometry/Sphere.hpp"

namespace rt {
Sphere::Sphere(double r, std::shared_ptr<const IMaterial> m) : Shape(std::move(m)), radius_(r) {}

bool Sphere::intersect(const Ray &r, double lo, double hi, Hit &h) const {
    double a = r.direction.length2(), b = dot(r.origin, r.direction),
           c = r.origin.length2() - radius_ * radius_, d = b * b - a * c;
    if (d < 0)
        return false;
    double t = (-b - std::sqrt(d)) / a;
    if (t < lo || t > hi)
        t = (-b + std::sqrt(d)) / a;
    return finish(r, t, r.at(t) / radius_, lo, hi, h);
}

Bounds Sphere::bounds() const {
    return {{-radius_, -radius_, -radius_}, {radius_, radius_, radius_}};
}
} // namespace rt
