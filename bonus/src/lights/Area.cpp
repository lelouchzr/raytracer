#include "rt/lights/Area.hpp"
#include "rt/Extensions.hpp"

namespace rt {
Area::Area(const Json &j)
    : center_(j.at("position").vector()), u_(j.at("u").vector()), v_(j.at("v").vector()),
      normal_(cross(u_, v_)),
      radiance_(colorValue(j.vector("color", {1, 1, 1})) *
                range(j.number("intensity", 1), 0, 10000, "area intensity")),
      area_(std::sqrt(normal_.length2())), uu_(dot(u_, u_)), uv_(dot(u_, v_)), vv_(dot(v_, v_)) {
    keys(j, {"type", "position", "u", "v", "color", "intensity", "twoSided"});
    range(area_, 1e-8, 1e12, "area light surface");
    if (uu_ * vv_ - uv_ * uv_ <= uu_ * vv_ * 1e-12)
        throw std::runtime_error("area light edges are nearly parallel");
    normal_ = normal_ / area_;
    if (j.has("twoSided")) {
        auto flag = std::get_if<bool>(&j.at("twoSided").value);
        if (!flag)
            throw std::runtime_error("area twoSided must be boolean");
        twoSided_ = *flag;
    }
}

LightSample Area::towards(Vec3 point, Vec3 target) const {
    Vec3 delta = target - point;
    double distance2 = delta.length2(), distance = std::sqrt(distance2);
    if (distance < epsilon)
        return {{0, 1, 0}, {}, distance, false};
    Vec3 direction = delta / distance;
    double cosine = dot(normal_, -direction);
    cosine = twoSided_ ? std::abs(cosine) : std::max(0., cosine);
    // Uniform area density 1/A converted to solid angle: p(w) = r² / (A cosLight).
    return {direction, radiance_ * (area_ * cosine / distance2), distance, false};
}

LightSample Area::sample(Vec3 point) const {
    return towards(point, center_);
}

LightSample Area::sample(Vec3 point, unsigned long long &rng) const {
    double a = random01(rng) - .5, b = random01(rng) - .5;
    return towards(point, center_ + u_ * a + v_ * b);
}

bool Area::intersect(const Ray &ray, double lo, double &hi, Vec3 &emission) const {
    double denominator = dot(ray.direction, normal_);
    if (std::abs(denominator) < 1e-12)
        return false;
    double t = dot(center_ - ray.origin, normal_) / denominator;
    if (t < lo || t >= hi)
        return false;
    Vec3 delta = ray.at(t) - center_;
    double du = dot(delta, u_), dv = dot(delta, v_), determinant = area_ * area_;
    double a = (du * vv_ - dv * uv_) / determinant;
    double b = (dv * uu_ - du * uv_) / determinant;
    if (std::abs(a) > .5 || std::abs(b) > .5)
        return false;
    hi = t;
    // The unlit back is still an opaque surface.
    emission = twoSided_ || denominator < 0 ? radiance_ : Vec3{};
    return true;
}

bool Area::emit(unsigned long long &rng, Ray &ray, Vec3 &power) const {
    double a = random01(rng) - .5, b = random01(rng) - .5;
    Vec3 normal = normal_;
    if (twoSided_ && random01(rng) < .5)
        normal = -normal;
    ray = {center_ + u_ * a + v_ * b + normal * epsilon, hemisphere(normal, rng)};
    power = radiance_ * (pi * area_ * (twoSided_ ? 2 : 1));
    return true;
}
} // namespace rt
