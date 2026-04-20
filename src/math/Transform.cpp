#include "rt/math.hpp"

namespace rt {
Vec3 Transform::rotate(Vec3 v, bool inverse) const {
    auto axis = [](Vec3 p, int a, double d) {
        double c = std::cos(d), s = std::sin(d);
        if (a == 0)
            return Vec3{p.x, c * p.y - s * p.z, s * p.y + c * p.z};
        if (a == 1)
            return Vec3{c * p.x + s * p.z, p.y, -s * p.x + c * p.z};
        return Vec3{c * p.x - s * p.y, s * p.x + c * p.y, p.z};
    };
    for (int k = 0; k < 3; ++k) {
        int a = inverse ? 2 - k : k;
        v = axis(v, a, rotation[a] * pi / 180 * (inverse ? -1 : 1));
    }
    return v;
}

Vec3 Transform::point(Vec3 v) const {
    return rotate(v) + translation;
}

Vec3 Transform::local(Vec3 v) const {
    return rotate(v - translation, true);
}
} // namespace rt
