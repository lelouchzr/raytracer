#include "rt/lights/Point.hpp"
#include "rt/Extensions.hpp"

namespace rt {
Point::Point(const Json &j)
    : position_(j.at("position").vector()),
      radiance_(colorValue(j.vector("color", {1, 1, 1})) *
                range(j.number("intensity", 1), 0, 10000, "point intensity")),
      falloff_(range(j.number("falloff", 0), 0, 100, "point falloff")), physical_(false) {
    keys(j, {"type", "position", "color", "intensity", "falloff", "physical"});
    if (j.has("physical")) {
        auto flag = std::get_if<bool>(&j.at("physical").value);
        if (!flag)
            throw std::runtime_error("point physical must be boolean");
        physical_ = *flag;
    }
}

LightSample Point::sample(Vec3 p) const {
    Vec3 d = position_ - p;
    double distance = std::sqrt(d.length2());
    return {distance < epsilon ? Vec3{0, 1, 0} : d / distance,
            radiance_ / (physical_ ? std::max(epsilon * epsilon, distance * distance)
                                   : 1 + falloff_ * distance * distance),
            distance, false};
}
} // namespace rt

namespace rt {
bool Point::emit(unsigned long long &rng, Ray &ray, Vec3 &power) const {
    double y = 2 * random01(rng) - 1, a = 2 * pi * random01(rng), radius = std::sqrt(1 - y * y);
    ray = {position_, {radius * std::cos(a), y, radius * std::sin(a)}};
    power = radiance_ * (4 * pi);
    return true;
}
} // namespace rt
