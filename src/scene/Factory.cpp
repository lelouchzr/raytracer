#ifdef RT_BONUS
#include "rt/transform/AffineTransform.hpp"
#endif
#include "rt/engine.hpp"
#include "rt/geometry/Plane.hpp"
#include "rt/geometry/Quadric.hpp"
#include "rt/geometry/Sphere.hpp"
#include "rt/lights/Ambient.hpp"
#include "rt/lights/Directional.hpp"

namespace rt {
Factory::Factory() {
    shapes["sphere"] = [](const Json &j, auto m) {
        keys(j, {"type", "radius", "material", "translate", "rotate"});
        return std::make_shared<Sphere>(range(j.number("radius", 1), 1e-6, 1e6, "radius"), m);
    };
    shapes["plane"] = [](const Json &j, auto m) {
        keys(j, {"type", "normal", "material", "translate", "rotate"});
        return std::make_shared<Plane>(j.vector("normal", {0, 1, 0}), m);
    };
    for (bool cone : {false, true})
        shapes[cone ? "cone" : "cylinder"] = [cone](const Json &j, auto m) {
            keys(j, {"type", "radius", "material", "translate", "rotate"});
            return std::make_shared<Quadric>(range(j.number("radius", 1), 1e-6, 1e6, "radius"),
                                             cone, m);
        };
    materials["flat"] = [](const Json &j, const auto &) {
        keys(j, {"type", "color"});
        return std::make_shared<FlatMaterial>(colorValue(j.vector("color", {.7, .7, .7})));
    };
    lights["ambient"] = [](const Json &j) {
        keys(j, {"type", "color", "intensity"});
        return std::make_shared<Ambient>(
            colorValue(j.vector("color", {1, 1, 1})) *
            range(j.number("intensity", .15), 0, 100, "light intensity"));
    };
    lights["directional"] = [](const Json &j) {
        keys(j, {"type", "color", "intensity", "direction"});
        return std::make_shared<Directional>(
            j.at("direction").vector(),
            colorValue(j.vector("color", {1, 1, 1})) *
                range(j.number("intensity", 1), 0, 100, "light intensity"));
    };
}

Primitive Factory::primitive(const Json &j, std::shared_ptr<const IMaterial> m) const {
    auto type = j.at("type").string();
    auto it = shapes.find(type);
    if (it == shapes.end())
        throw std::runtime_error("unknown primitive '" + type + "'");
    Json definition = j;
#ifdef RT_BONUS
    for (auto name : {"scale", "shear", "matrix"})
        std::get<Json::Object>(definition.value).erase(name);
#endif
    auto shape = it->second(definition, std::move(m));
#ifdef RT_BONUS
    return applyAffine(std::move(shape), j);
#endif
    Transform t{j.vector("translate", {}), j.vector("rotate", {})};
    return std::make_shared<Transformed>(std::move(shape), t);
}
} // namespace rt
