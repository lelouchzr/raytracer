#include "rt/Extensions.hpp"
#include "rt/geometry/Box.hpp"
#include "rt/geometry/FiniteQuadric.hpp"
#include "rt/geometry/Triangle.hpp"

namespace rt {
void registerGeometry(Factory &f) {
    f.shapes["triangle"] = [](const Json &j, auto m) {
        keys(j, {"type", "a", "b", "c", "material", "translate", "rotate"});
        return std::make_shared<Triangle>(j.at("a").vector(), j.at("b").vector(),
                                          j.at("c").vector(), m);
    };
    f.shapes["box"] = [](const Json &j, auto m) {
        keys(j, {"type", "size", "material", "translate", "rotate"});
        Vec3 size = j.at("size").vector();
        for (int i = 0; i < 3; ++i)
            range(size[i], 1e-6, 1e6, "box size");
        return std::make_shared<Box>(size * .5, m);
    };
    for (bool cone : {false, true})
        f.shapes[cone ? "limited_cone" : "limited_cylinder"] = [cone](const Json &j, auto m) {
            keys(j, {"type", "radius", "height", "material", "translate", "rotate"});
            return std::make_shared<FiniteQuadric>(
                range(j.number("radius", 1), 1e-6, 1e6, "radius"),
                range(j.number("height", 2), 1e-6, 1e6, "height"), cone, m);
        };
}

} // namespace rt
