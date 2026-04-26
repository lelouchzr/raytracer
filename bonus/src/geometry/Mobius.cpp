#include "rt/geometry/Mobius.hpp"
#include "rt/Extensions.hpp"
#include "rt/geometry/Triangle.hpp"

namespace rt {
Mobius::Mobius(const Json &j, std::shared_ptr<const IMaterial> m) {
    keys(j, {"type", "material", "radius", "width", "segments", "translate", "rotate"});
    double radius = range(j.number("radius", 1.5), .1, 10, "Mobius radius"),
           width = range(j.number("width", .5), .01, 2, "Mobius half-width");
    int segments = integer(j.number("segments", 128), 16, 1024, "Mobius segments");
    std::vector<Primitive> triangles;
    auto point = [&](double angle, double side) {
        return Vec3{(radius + side * std::cos(angle * .5)) * std::cos(angle),
                    side * std::sin(angle * .5),
                    (radius + side * std::cos(angle * .5)) * std::sin(angle)};
    };
    for (int i = 0; i < segments; ++i) {
        double a = 2 * pi * i / segments, b = 2 * pi * (i + 1) / segments;
        Vec3 p = point(a, -width), q = point(a, width), r = point(b, width), s = point(b, -width);
        triangles.push_back(std::make_shared<Triangle>(p, q, r, m));
        triangles.push_back(std::make_shared<Triangle>(p, r, s, m));
    }
    mesh_ = buildBvh(std::move(triangles));
}

bool Mobius::intersect(const Ray &r, double lo, double hi, Hit &h) const {
    return mesh_->intersect(r, lo, hi, h);
}

Bounds Mobius::bounds() const {
    return mesh_->bounds();
}

void registerMobius(Factory &f) {
    f.shapes["mobius"] = [](const Json &j, auto m) {
        return std::make_shared<Mobius>(j, std::move(m));
    };
}
} // namespace rt
