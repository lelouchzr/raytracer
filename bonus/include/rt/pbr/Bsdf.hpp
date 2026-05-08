#pragma once
#include "rt/engine.hpp"

namespace rt {
struct BsdfSample {
    Vec3 direction, weight;
    double pdf = 0;
    bool delta = false, transmitted = false;
};

class Bsdf {
    Surface material_;
    Vec3 normal_, tangent_, bitangent_;
    double etaI_, etaT_, alpha_;
    Vec3 local(Vec3) const;
    Vec3 world(Vec3) const;
    double distribution(Vec3) const;
    double masking(Vec3) const;
    Vec3 microfacet(Vec3, unsigned long long &) const;
    Vec3 fresnel(double) const;
    double diffuseProbability() const;
    double reflectionProbability(Vec3, Vec3) const;

  public:
    Bsdf(Surface, Vec3 normal, double etaI = 1, double etaT = 1.5);
    static double dielectricFresnel(double cosine, double etaI, double etaT);
    Vec3 evaluate(Vec3 outgoing, Vec3 incoming) const;
    double pdf(Vec3 outgoing, Vec3 incoming) const;
    BsdfSample sample(Vec3 outgoing, unsigned long long &) const;
};
} // namespace rt
