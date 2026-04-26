#include "rt/geometry/FiniteQuadric.hpp"
#include "rt/Extensions.hpp"

namespace rt {
FiniteQuadric::FiniteQuadric(double r, double height, bool cone, std::shared_ptr<const IMaterial> m)
    : Shape(std::move(m)), radius_(r), height_(height), cone_(cone) {}

bool FiniteQuadric::intersect(const Ray &r, double lo, double hi, Hit &h) const {
    const double k = cone_ ? radius_ * radius_ / (height_ * height_) : 0;
    const double oy = r.origin.y - height_;
    double a = r.direction.x * r.direction.x + r.direction.z * r.direction.z -
               k * r.direction.y * r.direction.y;
    double b =
        2 * (r.origin.x * r.direction.x + r.origin.z * r.direction.z - k * oy * r.direction.y);
    double c = r.origin.x * r.origin.x + r.origin.z * r.origin.z - k * oy * oy -
               (cone_ ? 0 : radius_ * radius_);
    bool found = false;
    auto test = [&](double t, Vec3 n) {
        if (n.length2() > 1e-20 && finish(r, t, n, lo, hi, h)) {
            hi = t;
            found = true;
        }
    };
    auto side = [&](double t) {
        Vec3 p = r.at(t);
        if (p.y >= 0 && p.y <= height_)
            test(t, {p.x, -k * (p.y - height_), p.z});
    };
    if (std::abs(a) > 1e-12) {
        double d = b * b - 4 * a * c;
        if (d >= 0) {
            side((-b - std::sqrt(d)) / (2 * a));
            side((-b + std::sqrt(d)) / (2 * a));
        }
    } else if (std::abs(b) > 1e-12)
        side(-c / b);
    if (std::abs(r.direction.y) > 1e-12)
        for (int cap = 0; cap < (cone_ ? 1 : 2); ++cap) {
            double t = (cap * height_ - r.origin.y) / r.direction.y;
            Vec3 p = r.at(t);
            if (p.x * p.x + p.z * p.z <= radius_ * radius_)
                test(t, {0, cap ? 1. : -1., 0});
        }
    return found;
}

Bounds FiniteQuadric::bounds() const {
    return {{-radius_, 0, -radius_}, {radius_, height_, radius_}};
}
} // namespace rt
