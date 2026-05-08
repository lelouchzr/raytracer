#include "rt/Extensions.hpp"
#include "rt/geometry/Implicit.hpp"
#include "rt/geometry/Mobius.hpp"
#include "rt/lights/Area.hpp"
#include "rt/lights/Point.hpp"
#include "rt/materials/AdvancedMaterial.hpp"
#include "rt/pbr/PhysicalIntegrator.hpp"
#include "rt/render/AdvancedIntegrator.hpp"
#include <fstream>

namespace rt {

void registerMaterials(Factory &f) {
    f.materials["phong"] = [](const Json &j, const auto &base) {
        return std::make_shared<AdvancedMaterial>(j, base);
    };
    f.materials["pbr"] = f.materials["phong"];
    f.lights["point"] = [](const Json &j) { return std::make_shared<Point>(j); };
    f.lights["area"] = [](const Json &j) { return std::make_shared<Area>(j); };
}

void registerBonus(Factory &f) {
    registerGeometry(f);
    registerImplicit(f);
    registerMobius(f);
    registerMaterials(f);
}

void configureBonus(Scene &s, bool preparePhotons) {
    s.acceleration = buildBvh(s.objects);
    if (s.settings.integrator == "pbr") {
        s.integrator = std::make_shared<PhysicalIntegrator>(s);
        return;
    }
    s.integrator = std::make_shared<AdvancedIntegrator>(
        s, preparePhotons && s.settings.integrator == "photon" ? std::make_shared<PhotonMap>(s)
                                                               : nullptr);
}
} // namespace rt
