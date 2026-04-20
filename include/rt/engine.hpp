#pragma once
#include "rt/geometry/Hit.hpp"
#include "rt/geometry/IPrimitive.hpp"
#include "rt/geometry/Shape.hpp"
#include "rt/lights/ILight.hpp"
#include "rt/lights/LightSample.hpp"
#include "rt/materials/FlatMaterial.hpp"
#include "rt/materials/IMaterial.hpp"
#include "rt/materials/Surface.hpp"
#include "rt/render/IIntegrator.hpp"
#include "rt/render/Settings.hpp"
#include "rt/scene/Camera.hpp"
#include "rt/scene/CameraFrame.hpp"
#include "rt/scene/Factory.hpp"
#include "rt/scene/Scene.hpp"
#include "rt/scene/SceneBuilder.hpp"
#include "rt/transform/Transformed.hpp"

namespace rt {
std::shared_ptr<const IIntegrator> basicIntegrator();
Vec3 colorValue(Vec3);
void writePpm(const Scene &, const std::vector<Vec3> &);
void render(const Scene &);
#ifdef RT_BONUS
Primitive applyScale(Primitive, Vec3);
void registerBonus(Factory &);
void configureBonus(Scene &, bool preparePhotons = true);
void bonusRender(const Scene &);
Primitive makeMesh(const Json &, std::shared_ptr<const IMaterial>, const std::filesystem::path &);
#endif
} // namespace rt
