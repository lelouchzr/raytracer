#pragma once
#include "rt/engine.hpp"

namespace rt {
Vec3 hemisphere(Vec3, unsigned long long &);
Vec3 visibility(const Scene &, Ray, double, bool transparent = true);
void registerGeometry(Factory &);
void registerMaterials(Factory &);
Primitive buildBvh(std::vector<Primitive>);
std::shared_ptr<const IIntegrator> advancedIntegrator();

double random01(unsigned long long &);
} // namespace rt
