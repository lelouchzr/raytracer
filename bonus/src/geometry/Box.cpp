#include "rt/geometry/Box.hpp"
#include "rt/Extensions.hpp"

namespace rt {
Box::Box(Vec3 half, std::shared_ptr<const IMaterial> m) : Shape(std::move(m)), half_(half) {}

bool Box::intersect(const Ray &r, double lo, double hi, Hit &h) const {
    double near = -infinity, far = infinity;
    Vec3 nearN, farN;
    const std::array<double, 3> origin{r.origin.x, r.origin.y, r.origin.z};
    const std::array<double, 3> direction{r.direction.x, r.direction.y, r.direction.z};
    const std::array<double, 3> half{half_.x, half_.y, half_.z};
    for (std::size_t axis = 0; axis < origin.size(); ++axis) {
        if (std::abs(direction[axis]) < 1e-14) {
            if (std::abs(origin[axis]) > half[axis])
                return false;
            continue;
        }
        double a = (-half[axis] - origin[axis]) / direction[axis],
               b = (half[axis] - origin[axis]) / direction[axis];
        Vec3 n = axis == 0 ? Vec3{-1, 0, 0} : axis == 1 ? Vec3{0, -1, 0} : Vec3{0, 0, -1};
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
