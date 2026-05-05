#include "rt/math.hpp"

namespace rt {
void Bounds::include(Vec3 p) {
    min = {std::min(min.x, p.x), std::min(min.y, p.y), std::min(min.z, p.z)};
    max = {std::max(max.x, p.x), std::max(max.y, p.y), std::max(max.z, p.z)};
}

void Bounds::include(Bounds b) {
    include(b.min);
    include(b.max);
}

bool Bounds::finite() const {
    return std::isfinite(min.x) && std::isfinite(min.y) && std::isfinite(min.z) &&
           std::isfinite(max.x) && std::isfinite(max.y) && std::isfinite(max.z);
}

bool Bounds::hit(Ray r, double lo, double hi) const {
    const std::array<double, 3> origin{r.origin.x, r.origin.y, r.origin.z};
    const std::array<double, 3> direction{r.direction.x, r.direction.y, r.direction.z};
    const std::array<double, 3> lower{min.x, min.y, min.z}, upper{max.x, max.y, max.z};
    for (std::size_t axis = 0; axis < origin.size(); ++axis) {
        if (std::abs(direction[axis]) < 1e-14) {
            if (origin[axis] < lower[axis] || origin[axis] > upper[axis])
                return false;
            continue;
        }
        double a = (lower[axis] - origin[axis]) / direction[axis],
               b = (upper[axis] - origin[axis]) / direction[axis];
        if (a > b)
            std::swap(a, b);
        lo = std::max(lo, a);
        hi = std::min(hi, b);
        if (hi < lo)
            return false;
    }
    return true;
}
} // namespace rt
