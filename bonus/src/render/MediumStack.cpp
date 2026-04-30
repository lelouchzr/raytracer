#include "rt/render/MediumStack.hpp"
#include "rt/engine.hpp"
#include <set>
#include <tuple>

namespace rt {
MediumStack MediumStack::atOrigin(const Scene &scene, Vec3 origin) {
    std::vector<std::tuple<double, const void *, double>> enclosing;
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
            enclosing.emplace_back(distance, h.boundary, material.ior);
        probe.origin = h.point + probe.direction * epsilon;
        distance += epsilon;
    }
    std::sort(enclosing.begin(), enclosing.end(),
              [](const auto &a, const auto &b) { return std::get<0>(a) > std::get<0>(b); });
    MediumStack stack;
    for (auto [distanceToExit, boundary, ior] : enclosing) {
        (void)distanceToExit;
        stack.media_.emplace_back(boundary, ior);
    }
    return stack;
}

double MediumStack::current() const {
    return media_.empty() ? 1 : media_.back().second;
}

MediumStack MediumStack::cross(const Hit &h, double ior) const {
    auto next = *this;
    if (h.front)
        next.media_.emplace_back(h.boundary, ior);
    else
        for (auto it = next.media_.rbegin(); it != next.media_.rend(); ++it)
            if (it->first == h.boundary) {
                next.media_.erase(std::next(it).base());
                break;
            }
    return next;
}

std::pair<double, double> MediumStack::indices(const Hit &h, double ior) const {
    double incident = current();
    if (!h.front && media_.empty())
        incident = ior;
    return {incident, cross(h, ior).current()};
}
} // namespace rt
