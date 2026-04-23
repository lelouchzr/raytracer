#include "rt/render/BasicIntegrator.hpp"

namespace rt {
Vec3 BasicIntegrator::trace(const Scene &s, Ray ray, int, unsigned long long &) const {
    Hit h;
    if (!s.intersect(ray, epsilon, infinity, h))
        return s.background;
    Vec3 color;
    const auto surface = h.material->sample(h.local);
    for (const auto &light : s.lights) {
        auto l = light->sample(h.point);
        if (l.ambient) {
            color += surface.color * l.radiance;
            continue;
        }
        Hit shadow;
        if (!s.intersect({h.point + h.normal * epsilon, l.direction}, epsilon, l.distance - epsilon,
                         shadow))
            color += surface.color * l.radiance * std::max(0., dot(h.normal, l.direction));
    }
    return color;
}
} // namespace rt
