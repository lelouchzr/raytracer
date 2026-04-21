#include "rt/geometry/Quadric.hpp"

namespace rt {
Quadric::Quadric(double r, bool cone, std::shared_ptr<const IMaterial> m)
    : Shape(std::move(m)), radius_(r), cone_(cone) {}

bool Quadric::intersect(const Ray &r, double lo, double hi, Hit &h) const {
    double k = cone_ ? radius_ * radius_ : 0;
    double a = r.direction.x * r.direction.x + r.direction.z * r.direction.z -
               k * r.direction.y * r.direction.y;
    double b = 2 * (r.origin.x * r.direction.x + r.origin.z * r.direction.z -
                    k * r.origin.y * r.direction.y);
    double c = r.origin.x * r.origin.x + r.origin.z * r.origin.z - k * r.origin.y * r.origin.y -
               (cone_ ? 0 : radius_ * radius_);
    double t1 = infinity, t2 = infinity;
    if (std::abs(a) < 1e-12) {
        if (std::abs(b) < 1e-12)
            return false;
        t1 = -c / b;
    } else {
        double d = b * b - 4 * a * c;
        if (d < 0)
            return false;
        t1 = (-b - std::sqrt(d)) / (2 * a);
        t2 = (-b + std::sqrt(d)) / (2 * a);
        if (t1 > t2)
            std::swap(t1, t2);
    }
    double t = (t1 >= lo && t1 <= hi) ? t1 : t2;
    Vec3 p = r.at(t);
    Vec3 n{p.x, -k * p.y, p.z};
    if (n.length2() < 1e-20)
        return false;
    return finish(r, t, n, lo, hi, h);
}

Bounds Quadric::bounds() const {
    return {{-infinity, -infinity, -infinity}, {infinity, infinity, infinity}};
}
} // namespace rt
