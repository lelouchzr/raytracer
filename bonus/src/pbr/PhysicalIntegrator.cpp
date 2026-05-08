#include "rt/pbr/PhysicalIntegrator.hpp"
#include "rt/Extensions.hpp"
#include "rt/pbr/Bsdf.hpp"

namespace rt {
PhysicalIntegrator::PhysicalIntegrator(const Scene &scene)
    : initial_(MediumStack::atOrigin(scene, scene.camera.position)),
      origin_(scene.camera.position) {}

double PhysicalIntegrator::powerWeight(double first, double second) {
    double maximum = std::max(first, second);
    if (maximum <= 0)
        return 0;
    first /= maximum;
    second /= maximum;
    return first * first / (first * first + second * second);
}

Vec3 PhysicalIntegrator::trace(const Scene &scene, Ray ray, int firstDepth,
                               unsigned long long &rng) const {
    MediumStack media = (ray.origin - origin_).length2() < 1e-20
                            ? initial_
                            : MediumStack::atOrigin(scene, ray.origin);
    Vec3 result, throughput{1, 1, 1}, previousPoint;
    double previousPdf = 0, etaScale = 1;
    bool specular = true;
    for (int depth = firstDepth; depth < scene.settings.depth; ++depth) {
        Hit hit;
        bool found = scene.intersect(ray, epsilon, infinity, hit);
        double nearest = found ? hit.t : infinity;
        const ILight *emitter = nullptr;
        Vec3 emitted;
        for (const auto &light : scene.lights)
            if (light->intersect(ray, epsilon, nearest, emitted))
                emitter = light.get();
        if (emitter) {
            double weight =
                specular
                    ? 1
                    : powerWeight(previousPdf, emitter->directionPdf(previousPoint, ray.direction));
            result += throughput * media.transmittance(nearest) * emitted * weight;
            break;
        }
        if (!found) {
            if (scene.environmentLight) {
                double weight = specular
                                    ? 1
                                    : powerWeight(previousPdf, scene.environmentLight->directionPdf(
                                                                   previousPoint, ray.direction));
                result += throughput * scene.environmentLight->environment(ray.direction) * weight;
            } else
                result += throughput * scene.background;
            break;
        }
        throughput = throughput * media.transmittance(hit.t);
        prepareFootprint(ray, hit);
        Surface material = hit.material->sampleHit(hit);
        result += throughput * material.emission;
        if (!material.physical)
            material.roughness = material.transmission > 0
                                     ? 0
                                     : std::clamp(std::sqrt(2 / (material.shininess + 2)), .08, 1.);
        Vec3 normal = hit.material->shadingNormal(hit), outgoing = -ray.direction;
        auto [etaI, etaT] = media.indices(hit, material.ior);
        // Opaque surfaces do not enter the volume stack, but still have a dielectric interface.
        if (material.transmission == 0)
            etaT = material.ior;
        Bsdf bsdf(material, normal, etaI, etaT);
        for (const auto &light : scene.lights) {
            auto sample = light->sample(hit.point, rng);
            if (sample.ambient || sample.radiance.length2() == 0)
                continue;
            double geometricCos = dot(hit.geometric, sample.direction);
            // A connection through another geometric boundary is occluded; that path
            // requires continuation and refraction at the intervening interface.
            if (geometricCos <= 0 && material.transmission == 0)
                continue;
            Vec3 f = bsdf.evaluate(outgoing, sample.direction);
            if (f.length2() == 0)
                continue;
            Vec3 visible = visibility(scene, spawnRay(hit, sample.direction), sample.distance,
                                      false, light.get());
            double weight =
                sample.pdf > 0 ? powerWeight(sample.pdf, bsdf.pdf(outgoing, sample.direction)) : 1;
            auto shadowMedium =
                geometricCos < 0 ? media.cross(hit, material.ior, material.absorption) : media;
            result += throughput * f * sample.radiance * visible *
                      shadowMedium.transmittance(sample.distance) *
                      (std::abs(dot(normal, sample.direction)) * weight);
        }
        auto next = bsdf.sample(outgoing, rng);
        if (next.pdf <= 0 || next.weight.length2() == 0)
            break;
        if ((dot(hit.geometric, next.direction) < 0) != next.transmitted)
            break;
        throughput = throughput * next.weight;
        if (!std::isfinite(throughput.length2()))
            throw std::runtime_error("non-finite BSDF throughput");
        if (next.transmitted) {
            media = media.cross(hit, material.ior, material.absorption);
            etaScale *= (etaT * etaT) / (etaI * etaI);
        }
        previousPoint = hit.point;
        previousPdf = next.pdf;
        specular = next.delta;
        ray = spawnRay(hit, next.direction);
        if (depth >= 3) {
            double survive = std::clamp(
                std::max({throughput.x, throughput.y, throughput.z}) * etaScale, .05, .95);
            if (random01(rng) >= survive)
                break;
            throughput = throughput / survive;
        }
    }
    return result;
}
} // namespace rt
