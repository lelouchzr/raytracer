#pragma once
#include "rt/engine.hpp"

namespace rt {
Vec3 hemisphere(Vec3, unsigned long long &);
void prepareFootprint(const Ray &, Hit &);
Ray spawnRay(const Hit &, Vec3);
bool intersectEmitter(const Scene &, const Ray &, double &, Vec3 &, const ILight *skip = nullptr);
Vec3 visibility(const Scene &, Ray, double, bool transparent = true, const ILight *skip = nullptr);
void registerGeometry(Factory &);
void registerMaterials(Factory &);
Primitive buildBvh(std::vector<Primitive>);
std::shared_ptr<const IIntegrator> advancedIntegrator();

double random01(unsigned long long &);
} // namespace rt
