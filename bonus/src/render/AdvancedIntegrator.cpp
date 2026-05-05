#include "rt/render/AdvancedIntegrator.hpp"
#include "rt/Extensions.hpp"

namespace rt {
AdvancedIntegrator::AdvancedIntegrator(std::shared_ptr<const PhotonMap> photons)
    : photons_(std::move(photons)) {}

AdvancedIntegrator::AdvancedIntegrator(const Scene &scene, std::shared_ptr<const PhotonMap> photons)
    : photons_(std::move(photons)), initial_(MediumStack::atOrigin(scene, scene.camera.position)),
      origin_(scene.camera.position) {}

Vec3 AdvancedIntegrator::trace(const Scene &s, Ray ray, int depth, unsigned long long &rng) const {
    return radiance(s, ray, depth, rng,
                    (ray.origin - origin_).length2() < 1e-20 ? initial_
                                                             : MediumStack::atOrigin(s, ray.origin),
                    true);
}

Vec3 AdvancedIntegrator::radiance(const Scene &s, Ray ray, int depth, unsigned long long &rng,
                                  const MediumStack &media, bool emitted) const {
    if (depth >= s.settings.depth)
        return {};
    Hit h;
    if (!s.intersect(ray, epsilon, infinity, h))
        return s.background;
    Surface m = h.material->sampleHit(h);
    Vec3 normal = h.material->shadingNormal(h), direct;
    double diffuse = 1 - m.reflection - m.transmission;
    bool path = s.settings.integrator == "path";
    double ao = 1;
    if (s.settings.ao > 0 && depth == 0) {
        int visible = 0;
        for (int i = 0; i < s.settings.ao; ++i) {
            Hit shadow;
            if (!s.intersect({h.point + h.geometric * epsilon, hemisphere(h.geometric, rng)},
                             epsilon, 2, shadow))
                ++visible;
        }
        ao = .2 + .8 * double(visible) / s.settings.ao;
    }
    for (const auto &light : s.lights) {
        auto l = light->sample(h.point);
        if (l.ambient) {
            if (!path)
                direct += m.color * l.radiance * ao;
            continue;
        }
        double cosine = std::max(0., dot(normal, l.direction));
        if (cosine <= 0 || dot(h.geometric, l.direction) <= 0)
            continue;
        Vec3 trans = visibility(s, {h.point + h.geometric * epsilon, l.direction}, l.distance,
                                s.settings.integrator == "whitted");
        double specular =
            path ? 0
                 : m.specular *
                       std::pow(std::max(0., dot(reflect(-l.direction, normal), -ray.direction)),
                                m.shininess);
        direct += (m.color * (path ? cosine / pi : cosine) + Vec3{specular, specular, specular}) *
                  l.radiance * trans;
    }
    Vec3 result = (emitted ? m.emission : Vec3{}) + direct * diffuse;
    if (photons_ && diffuse > 0)
        result += photons_->radiance(h.point, h.geometric, m.color, h.boundary) * diffuse;
    double eta = 1, cosine = 0, sin2 = 0;
    double reflected = m.reflection, transmitted = 0;
    if (m.transmission > 0) {
        auto [etaI, etaT] = media.indices(h, m.ior);
        eta = etaI / etaT;
        cosine = std::clamp(dot(-ray.direction, h.geometric), 0., 1.);
        sin2 = eta * eta * (1 - cosine * cosine);
        double r0 = (etaI - etaT) / (etaI + etaT);
        r0 *= r0;
        double fresnel = sin2 > 1 ? 1 : r0 + (1 - r0) * std::pow(1 - cosine, 5);
        reflected = m.reflection + m.transmission * fresnel;
        transmitted = m.transmission * (1 - fresnel);
    }
    Vec3 reflectionTint = Vec3{1, 1, 1} * (1 - m.metallic) + m.color * m.metallic,
         transmissionTint = Vec3{.8, .8, .8} + m.color * .2;
    auto bounce = [&](Vec3 direction, const MediumStack &next) {
        direction = unit(direction);
        return radiance(s, {h.point + direction * epsilon, direction}, depth + 1, rng, next, true);
    };
    if (path) {
        double choice = random01(rng);
        double survive = depth < 3 ? 1 : .8;
        if (random01(rng) > survive)
            return result;
        if (choice < reflected)
            result += bounce(reflect(ray.direction, h.geometric), media) * reflectionTint / survive;
        else if (choice < reflected + transmitted) {
            Vec3 direction = eta * (ray.direction + cosine * h.geometric) -
                             std::sqrt(std::max(0., 1 - sin2)) * h.geometric;
            result +=
                bounce(direction, media.cross(h, m.ior)) * transmissionTint * (eta * eta) / survive;
        } else if (diffuse > 0) {
            Vec3 direction = hemisphere(normal, rng);
            if (dot(direction, h.geometric) > 0)
                result += bounce(direction, media) * m.color / survive;
        }
    } else {
        if (reflected > 0)
            result +=
                bounce(reflect(ray.direction, h.geometric), media) * reflectionTint * reflected;
        if (transmitted > 0) {
            Vec3 direction = eta * (ray.direction + cosine * h.geometric) -
                             std::sqrt(std::max(0., 1 - sin2)) * h.geometric;
            result += bounce(direction, media.cross(h, m.ior)) * transmissionTint * transmitted;
        }
    }
    return result;
}
} // namespace rt
