#include "rt/render/PhotonMap.hpp"
#include "rt/Extensions.hpp"
#include "rt/render/MediumStack.hpp"
#include "rt/render/RenderControl.hpp"

namespace rt {
PhotonMap::PhotonMap(const Scene &scene, const RenderControl *control)
    : radius_(scene.settings.photonRadius) {
    if (scene.settings.photons == 0)
        return;
    unsigned long long rng = 0x57cf5c41;
    std::vector<std::shared_ptr<const ILight>> emitters;
    for (const auto &light : scene.lights) {
        Ray test;
        Vec3 power;
        if (light->emit(rng, test, power))
            emitters.push_back(light);
    }
    if (emitters.empty())
        throw std::runtime_error("photon mapping requires a point emitter");
    for (int photon = 0; photon < scene.settings.photons; ++photon) {
        if (control && control->cancelled())
            break;
        auto &light = emitters[static_cast<std::size_t>(photon) % emitters.size()];
        Ray ray;
        Vec3 power;
        light->emit(rng, ray, power);
        power = power * (double(emitters.size()) / scene.settings.photons);
        MediumStack media = MediumStack::atOrigin(scene, ray.origin);
        bool specularChain = false;
        for (int bounce = 0; bounce < scene.settings.depth; ++bounce) {
            Hit hit;
            if (!scene.intersect(ray, epsilon, infinity, hit))
                break;
            Surface material = hit.material->sampleHit(hit);
            double diffuse = 1 - material.reflection - material.transmission;
            if (diffuse > 0 && specularChain) {
                photons_.push_back({hit.point, ray.direction, power, hit.boundary});
            }
            double pick = random01(rng), reflection = material.reflection,
                   transmission = material.transmission;
            Vec3 direction;
            auto [etaI, etaT] = media.indices(hit, material.ior);
            double eta = etaI / etaT,
                   cosine = std::clamp(dot(-ray.direction, hit.geometric), 0., 1.),
                   sine = eta * eta * (1 - cosine * cosine);
            double r0 = (etaI - etaT) / (etaI + etaT);
            r0 *= r0;
            double fresnel = sine > 1 ? 1 : r0 + (1 - r0) * std::pow(1 - cosine, 5);
            reflection += transmission * fresnel;
            transmission *= 1 - fresnel;
            if (pick < reflection) {
                direction = reflect(ray.direction, hit.geometric);
                power = power * (Vec3{1, 1, 1} * (1 - material.metallic) +
                                 material.color * material.metallic);
            } else if (pick < reflection + transmission) {
                direction = eta * (ray.direction + cosine * hit.geometric) -
                            std::sqrt(std::max(0., 1 - sine)) * hit.geometric;
                media = media.cross(hit, material.ior);
                power = power * (Vec3{.8, .8, .8} + material.color * .2);
            } else
                break;
            specularChain = true;
            direction = unit(direction);
            ray = {hit.point + direction * epsilon, direction};
        }
    }
    for (std::size_t i = 0; i < photons_.size(); ++i)
        grid_[cell(photons_[i].position)].push_back(i);
}

std::array<long long, 3> PhotonMap::cell(Vec3 p) const {
    return {static_cast<long long>(std::floor(std::clamp(p.x / radius_, -9e15, 9e15))),
            static_cast<long long>(std::floor(std::clamp(p.y / radius_, -9e15, 9e15))),
            static_cast<long long>(std::floor(std::clamp(p.z / radius_, -9e15, 9e15)))};
}

Vec3 PhotonMap::radiance(Vec3 point, Vec3 normal, Vec3 albedo, const void *receiver) const {
    Vec3 flux;
    double radius2 = radius_ * radius_;
    auto center = cell(point);
    for (int z = -1; z <= 1; ++z)
        for (int y = -1; y <= 1; ++y)
            for (int x = -1; x <= 1; ++x) {
                auto it = grid_.find({center[0] + x, center[1] + y, center[2] + z});
                if (it == grid_.end())
                    continue;
                for (auto index : it->second) {
                    const auto &photon = photons_[index];
                    if (receiver && photon.receiver != receiver)
                        continue;
                    double distance = (photon.position - point).length2();
                    if (distance < radius2 && dot(-photon.direction, normal) > 0)
                        flux += photon.power * (1 - distance / radius2);
                }
            }
    // Normalized Epanechnikov disk kernel, then Lambert BRDF 1/pi.
    return flux * albedo * (2 / (pi * pi * radius2));
}

std::size_t PhotonMap::size() const {
    return photons_.size();
}
} // namespace rt

namespace rt {
std::size_t PhotonMap::countFor(const void *receiver) const {
    return static_cast<std::size_t>(
        std::count_if(photons_.begin(), photons_.end(),
                      [&](const Photon &photon) { return photon.receiver == receiver; }));
}
} // namespace rt
