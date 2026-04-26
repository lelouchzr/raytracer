#include "rt/geometry/Box.hpp"
#include "rt/Extensions.hpp"

namespace rt {
Box::Box(Vec3 half, std::shared_ptr<const IMaterial> m) : Shape(std::move(m)), half_(half) {}

bool Box::intersect(const Ray &r, double lo, double hi, Hit &h) const {
    double near = -infinity, far = infinity;
    Vec3 nearN, farN;
    for (int i = 0; i < 3; ++i) {
        if (std::abs(r.direction[i]) < 1e-14) {
            if (std::abs(r.origin[i]) > half_[i])
                return false;
            continue;
        }
        double a = (-half_[i] - r.origin[i]) / r.direction[i],
               b = (half_[i] - r.origin[i]) / r.direction[i];
        Vec3 n = i == 0 ? Vec3{-1, 0, 0} : i == 1 ? Vec3{0, -1, 0} : Vec3{0, 0, -1};
        Vec3 n2 = -n;
        if (a > b) {
            std::swap(a, b);
            std::swap(n, n2);
        }
        if (a > near) {
            near = a;
            nearN = n;
        }
        if (b < far) {
            far = b;
            farN = n2;
        }
        if (near > far)
            return false;
    }
    return near >= lo ? finish(r, near, nearN, lo, hi, h) : finish(r, far, farN, lo, hi, h);
}

Bounds Box::bounds() const {
    return {-half_, half_};
}
} // namespace rt
