#include "rt/Extensions.hpp"
#include "rt/render/AdvancedIntegrator.hpp"

namespace rt {
Vec3 hemisphere(Vec3 n, unsigned long long &rng) {
    double a = 2 * pi * random01(rng), r = std::sqrt(random01(rng));
    Vec3 tangent = unit(cross(std::abs(n.y) < .9 ? Vec3{0, 1, 0} : Vec3{1, 0, 0}, n));
    return tangent * (r * std::cos(a)) + cross(n, tangent) * (r * std::sin(a)) +
           n * std::sqrt(1 - r * r);
}

void prepareFootprint(const Ray &ray, Hit &hit) {
    double cosine = std::max(.02, std::abs(dot(ray.direction, hit.geometric)));
    hit.footprint = (ray.coneWidth + ray.coneSpread * hit.t) / cosine;
    hit.coneSpread = ray.coneSpread;
}

Ray spawnRay(const Hit &hit, Vec3 direction) {
    direction = unit(direction);
    Vec3 offset = hit.geometric * (dot(direction, hit.geometric) >= 0 ? epsilon : -epsilon);
    return {hit.point + offset, direction, hit.footprint, hit.coneSpread};
}

bool intersectEmitter(const Scene &s, const Ray &ray, double &distance, Vec3 &emission,
                      const ILight *skip) {
    bool found = false;
    for (const auto &light : s.lights)
        if (light.get() != skip && light->intersect(ray, epsilon, distance, emission))
            found = true;
    return found;
}

Vec3 visibility(const Scene &s, Ray ray, double distance, bool transparent, const ILight *skip) {
    Vec3 transmittance{1, 1, 1};
    for (int i = 0; i < 24; ++i) {
        Hit h;
        bool found = s.intersect(ray, epsilon, distance - epsilon, h);
        double nearest = found ? h.t : distance - epsilon;
        Vec3 emission;
        if (intersectEmitter(s, ray, nearest, emission, skip))
            return {};
        if (!found)
            return transmittance;
        Surface m = h.material->sampleHit(h);
        if (!transparent || m.transmission <= 0)
            return {};
        transmittance = transmittance * (Vec3{.5, .5, .5} + m.color * .5) * m.transmission;
        if (transmittance.length2() < 1e-8)
            return {};
        distance -= h.t;
        ray = spawnRay(h, ray.direction);
    }
    return {};
}

std::shared_ptr<const IIntegrator> advancedIntegrator() {
    return std::make_shared<AdvancedIntegrator>();
}
} // namespace rt
