#include "PluginSphere.hpp"
#include "rt/lights/Directional.hpp"

namespace rt {
PluginSphere::PluginSphere(double radius, std::shared_ptr<const IMaterial> material)
    : Shape(std::move(material)), radius_(radius) {}

bool PluginSphere::intersect(const Ray &ray, double lo, double hi, Hit &h) const {
    double a = ray.direction.length2(), b = dot(ray.origin, ray.direction),
           c = ray.origin.length2() - radius_ * radius_, d = b * b - a * c;
    if (d < 0)
        return false;
    double t = (-b - std::sqrt(d)) / a;
    if (t < lo)
        t = (-b + std::sqrt(d)) / a;
    return finish(ray, t, ray.at(t) / radius_, lo, hi, h);
}

Bounds PluginSphere::bounds() const {
    return {{-radius_, -radius_, -radius_}, {radius_, radius_, radius_}};
}
} // namespace rt

extern "C" int raytracer_plugin_abi() {
    return 2;
}

extern "C" void raytracer_register(rt::Factory *factory) {
    factory->shapes["plugin_sphere"] = [](const rt::Json &j,
                                          std::shared_ptr<const rt::IMaterial> material) {
        rt::keys(j, {"type", "material", "radius", "translate", "rotate"});
        return std::make_shared<rt::PluginSphere>(
            rt::range(j.number("radius", 1), .001, 100, "plugin radius"), std::move(material));
    };
    factory->materials["plugin_material"] = [](const rt::Json &j, const std::filesystem::path &) {
        rt::keys(j, {"type", "color"});
        return std::make_shared<rt::FlatMaterial>(rt::colorValue(j.vector("color", {.2, .7, .8})));
    };
    factory->lights["plugin_light"] = [](const rt::Json &j) {
        rt::keys(j, {"type", "direction", "color", "intensity"});
        return std::make_shared<rt::Directional>(
            j.vector("direction", {-1, -2, -1}),
            rt::colorValue(j.vector("color", {1, 1, 1})) *
                rt::range(j.number("intensity", 1), 0, 100, "plugin intensity"));
    };
}
