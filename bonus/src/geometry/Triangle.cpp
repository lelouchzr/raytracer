#include "rt/geometry/Triangle.hpp"

namespace rt {
Triangle::Triangle(Vec3 a, Vec3 b, Vec3 c, std::shared_ptr<const IMaterial> m)
    : Triangle({a, b, c}, {}, {}, false, false, std::move(m)) {}

Triangle::Triangle(std::array<Vec3, 3> v, std::array<Vec3, 3> n, std::array<Vec3, 3> uv,
                   bool smooth, bool textured, std::shared_ptr<const IMaterial> m)
    : Shape(std::move(m)), vertices_(v), normals_(n), uvs_(uv),
      normal_(unit(cross(v[1] - v[0], v[2] - v[0]))), smooth_(smooth), textured_(textured) {}

bool Triangle::intersect(const Ray &ray, double lo, double hi, Hit &h) const {
    Vec3 e1 = vertices_[1] - vertices_[0], e2 = vertices_[2] - vertices_[0],
         p = cross(ray.direction, e2);
    double d = dot(e1, p);
    if (std::abs(d) < 1e-12)
        return false;
    Vec3 t = ray.origin - vertices_[0];
    double u = dot(t, p) / d;
    if (u < 0 || u > 1)
        return false;
    Vec3 q = cross(t, e1);
    double v = dot(ray.direction, q) / d;
    if (v < 0 || u + v > 1)
        return false;
    if (!finish(ray, dot(e2, q) / d, normal_, lo, hi, h))
        return false;
    if (smooth_) {
        Vec3 n = normals_[0] * (1 - u - v) + normals_[1] * u + normals_[2] * v;
        if (n.length2() > 1e-12) {
            h.normal = unit(n);
            if (dot(h.normal, h.geometric) < 0)
                h.normal = -h.normal;
        }
    }
    if (textured_) {
        h.hasUV = true;
        h.uv = uvs_[0] * (1 - u - v) + uvs_[1] * u + uvs_[2] * v;
        Vec3 a = uvs_[1] - uvs_[0], b = uvs_[2] - uvs_[0];
        double determinant = a.x * b.y - a.y * b.x;
        if (std::abs(determinant) > 1e-12) {
            h.tangent = unit((e1 * b.y - e2 * a.y) / determinant);
            h.bitangent = unit((e2 * a.x - e1 * b.x) / determinant);
        }
    }
    return true;
}

Bounds Triangle::bounds() const {
    Bounds b;
    for (auto v : vertices_)
        b.include(v);
    b.min = b.min - Vec3{epsilon, epsilon, epsilon};
    b.max = b.max + Vec3{epsilon, epsilon, epsilon};
    return b;
}
} // namespace rt
