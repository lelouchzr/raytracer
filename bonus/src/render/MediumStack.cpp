#include "rt/render/MediumStack.hpp"
#include "rt/engine.hpp"
#include <set>
#include <tuple>

namespace rt {
MediumStack MediumStack::atOrigin(const Scene &scene, Vec3 origin) {
    std::vector<std::tuple<double, const void *, double, Vec3>> enclosing;
    std::set<const void *> seen;
    Ray probe{origin, unit({.314159, .271828, .618034})};
    double distance = 0;
    for (int step = 0; step < 128; ++step) {
        Hit h;
        if (!scene.intersect(probe, epsilon, infinity, h))
            break;
        distance += h.t;
        Surface material = h.material->sampleHit(h);
        if (seen.insert(h.boundary).second && !h.front && material.transmission > 0)
            enclosing.emplace_back(distance, h.boundary, material.ior, material.absorption);
        probe.origin = h.point + probe.direction * epsilon;
        distance += epsilon;
    }
    std::sort(enclosing.begin(), enclosing.end(),
              [](const auto &a, const auto &b) { return std::get<0>(a) > std::get<0>(b); });
    MediumStack stack;
    for (auto [distanceToExit, boundary, ior, absorption] : enclosing) {
        (void)distanceToExit;
        stack.media_.push_back({boundary, ior, absorption});
    }
    return stack;
}

Vec3 MediumStack::transmittance(double distance) const {
    Vec3 sigma = media_.empty() ? Vec3{} : media_.back().absorption;
    return {sigma.x > 0 ? std::exp(-sigma.x * distance) : 1,
            sigma.y > 0 ? std::exp(-sigma.y * distance) : 1,
            sigma.z > 0 ? std::exp(-sigma.z * distance) : 1};
}

double MediumStack::current() const {
    return media_.empty() ? 1 : media_.back().ior;
}

MediumStack MediumStack::cross(const Hit &h, double ior, Vec3 absorption) const {
    auto next = *this;
    if (h.front)
        next.media_.push_back({h.boundary, ior, absorption});
    else
        for (auto it = next.media_.rbegin(); it != next.media_.rend(); ++it)
            if (it->boundary == h.boundary) {
                next.media_.erase(std::next(it).base());
                break;
            }
    return next;
}

std::pair<double, double> MediumStack::indices(const Hit &h, double ior) const {
    double incident = current();
    if (!h.front && media_.empty())
        incident = ior;
    if (h.front)
        return {incident, ior};
    // Removing a non-top or absent boundary leaves the current medium unchanged.
    if (!media_.empty() && media_.back().boundary == h.boundary)
        return {incident, media_.size() > 1 ? media_[media_.size() - 2].ior : 1};
    return {incident, current()};
}
} // namespace rt
