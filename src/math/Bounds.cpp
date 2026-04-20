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
    for (int i = 0; i < 3; ++i) {
        if (std::abs(r.direction[i]) < 1e-14) {
            if (r.origin[i] < min[i] || r.origin[i] > max[i])
                return false;
            continue;
        }
        double a = (min[i] - r.origin[i]) / r.direction[i],
               b = (max[i] - r.origin[i]) / r.direction[i];
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
