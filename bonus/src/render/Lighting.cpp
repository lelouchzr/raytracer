#include "rt/Extensions.hpp"
#include "rt/render/AdvancedIntegrator.hpp"

namespace rt {
Vec3 hemisphere(Vec3 n, unsigned long long &rng) {
    double a = 2 * pi * random01(rng), r = std::sqrt(random01(rng));
    Vec3 tangent = unit(cross(std::abs(n.y) < .9 ? Vec3{0, 1, 0} : Vec3{1, 0, 0}, n));
    return tangent * (r * std::cos(a)) + cross(n, tangent) * (r * std::sin(a)) +
           n * std::sqrt(1 - r * r);
}

Vec3 visibility(const Scene &s, Ray ray, double distance, bool transparent) {
    Vec3 transmittance{1, 1, 1};
    for (int i = 0; i < 24; ++i) {
        Hit h;
        if (!s.intersect(ray, epsilon, distance - epsilon, h))
            return transmittance;
        Surface m = h.material->sampleHit(h);
        if (!transparent || m.transmission <= 0)
            return {};
        transmittance = transmittance * (Vec3{.5, .5, .5} + m.color * .5) * m.transmission;
        if (transmittance.length2() < 1e-8)
            return {};
        distance -= h.t;
        ray.origin = h.point + ray.direction * epsilon;
    }
    return {};
}

std::shared_ptr<const IIntegrator> advancedIntegrator() {
    return std::make_shared<AdvancedIntegrator>();
}
} // namespace rt
