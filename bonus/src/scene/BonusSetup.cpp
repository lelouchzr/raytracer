#include "rt/Extensions.hpp"
#include "rt/geometry/Implicit.hpp"
#include "rt/geometry/Mobius.hpp"
#include "rt/lights/Point.hpp"
#include "rt/materials/AdvancedMaterial.hpp"
#include "rt/render/AdvancedIntegrator.hpp"
#include <fstream>

namespace rt {

void registerMaterials(Factory &f) {
    f.materials["phong"] = [](const Json &j, const auto &base) {
        return std::make_shared<AdvancedMaterial>(j, base);
    };
    f.lights["point"] = [](const Json &j) { return std::make_shared<Point>(j); };
}

void registerBonus(Factory &f) {
    registerGeometry(f);
    registerImplicit(f);
    registerMobius(f);
    registerMaterials(f);
}

void configureBonus(Scene &s, bool preparePhotons) {
    s.acceleration = buildBvh(s.objects);
    s.integrator = std::make_shared<AdvancedIntegrator>(
        s, preparePhotons && s.settings.integrator == "photon" ? std::make_shared<PhotonMap>(s)
                                                               : nullptr);
}
} // namespace rt
