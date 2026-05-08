#pragma once
#include "rt/geometry/IPrimitive.hpp"
#include "rt/lights/ILight.hpp"
#include "rt/render/IIntegrator.hpp"
#include "rt/scene/Camera.hpp"
#include <memory>
#include <vector>

namespace rt {
struct Scene {
    std::vector<std::shared_ptr<void>> modules;
    Camera camera;
    Settings settings;
    Vec3 background{0.025, 0.035, 0.06};
    std::vector<Primitive> objects;
    std::vector<std::shared_ptr<const ILight>> lights;
    std::shared_ptr<const IIntegrator> integrator;
    Primitive acceleration;
    std::shared_ptr<const ILight> environmentLight;
    bool intersect(Ray, double lo, double hi, Hit &) const;
};
} // namespace rt
