#include "rt/math.hpp"

namespace rt {
Vec3 operator*(double s, Vec3 v) {
    return v * s;
}

double dot(Vec3 a, Vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

Vec3 cross(Vec3 a, Vec3 b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

Vec3 unit(Vec3 v) {
    const double l = std::sqrt(v.length2());
    if (l < 1e-12)
        throw std::runtime_error("zero-length direction");
    return v / l;
}

Vec3 reflect(Vec3 v, Vec3 n) {
    return v - 2 * dot(v, n) * n;
}
} // namespace rt
