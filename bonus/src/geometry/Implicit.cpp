#include "rt/geometry/Implicit.hpp"
#include "rt/math/Polynomial.hpp"

namespace rt {
Implicit::Implicit(const Json &j, std::shared_ptr<const IMaterial> m)
    : Shape(std::move(m)), type_(j.at("type").string()),
      major_(range(j.number("major", 1.3), .1, 10, "major radius")),
      minor_(range(j.number("minor", .35), .01, 2, "minor radius")),
      iterations_(integer(j.number("iterations", 12), 2, 24, "fractal iterations")) {
    keys(j, {"type", "material", "major", "minor", "iterations", "translate", "rotate"});
}

double Implicit::field(Vec3 p) const {
    if (type_ == "torus")
        return std::hypot(std::hypot(p.x, p.z) - major_, p.y) - minor_;
    if (type_ == "tanglecube")
        return (std::pow(p.x, 4) - 5 * p.x * p.x + std::pow(p.y, 4) - 5 * p.y * p.y +
                std::pow(p.z, 4) - 5 * p.z * p.z + 11.8) /
               80.;
    Vec3 z = p;
    double derivative = 1, radius = 0;
    for (int i = 0; i < iterations_; ++i) {
        radius = std::sqrt(z.length2());
        if (radius > 2)
            break;
        if (radius < 1e-10)
            return -1e-5;
        double theta = std::acos(std::clamp(z.y / radius, -1., 1.)), phi = std::atan2(z.z, z.x);
        derivative = 8 * std::pow(radius, 7) * derivative + 1;
        double power = std::pow(radius, 8);
        theta *= 8;
        phi *= 8;
        z = Vec3{std::sin(theta) * std::cos(phi), std::cos(theta),
                 std::sin(theta) * std::sin(phi)} *
                power +
            p;
    }
    if (radius < 1e-10)
        return -1e-5;
    return .5 * std::log(radius) * radius / derivative;
}

Bounds Implicit::bounds() const {
    double extent = type_ == "torus" ? major_ + minor_ : type_ == "tanglecube" ? 2.5 : 1.5;
    return {{-extent, -extent, -extent}, {extent, extent, extent}};
}

bool Implicit::intersect(const Ray &r, double lo, double hi, Hit &hit) const {
    if (type_ == "torus" || type_ == "tanglecube") {
        std::array<double, 5> polynomial{};
        Vec3 o = r.origin, d = r.direction;
        if (type_ == "torus") {
            double a = d.length2(), b = 2 * dot(o, d),
                   c = o.length2() + major_ * major_ - minor_ * minor_, q = 4 * major_ * major_;
            polynomial = {c * c - q * (o.x * o.x + o.z * o.z),
                          2 * b * c - q * 2 * (o.x * d.x + o.z * d.z),
                          b * b + 2 * a * c - q * (d.x * d.x + d.z * d.z), 2 * a * b, a * a};
        } else {
            polynomial[0] = 11.8;
            for (int axis = 0; axis < 3; ++axis) {
                double a = o[axis], b = d[axis];
                polynomial[0] += a * a * a * a - 5 * a * a;
                polynomial[1] += 4 * a * a * a * b - 10 * a * b;
                polynomial[2] += 6 * a * a * b * b - 5 * b * b;
                polynomial[3] += 4 * a * b * b * b;
                polynomial[4] += b * b * b * b;
            }
        }
        Polynomial::RootBuffer roots;
        const auto rootCount = Polynomial::roots(polynomial, roots);
        for (std::size_t index = 0; index < rootCount; ++index) {
            const double root = roots[index];
            if (root >= lo && root <= hi) {
                Vec3 p = r.at(root), normal;
                if (type_ == "torus") {
                    double q = p.length2() + major_ * major_ - minor_ * minor_;
                    normal = {4 * p.x * (q - 2 * major_ * major_), 4 * p.y * q,
                              4 * p.z * (q - 2 * major_ * major_)};
                } else
                    normal = {4 * p.x * p.x * p.x - 10 * p.x, 4 * p.y * p.y * p.y - 10 * p.y,
                              4 * p.z * p.z * p.z - 10 * p.z};
                if (normal.length2() > 1e-20)
                    return finish(r, root, normal, lo, hi, hit);
            }
        }
        return false;
    }
    auto box = bounds();
    double start = lo, stop = hi;
    for (int axis = 0; axis < 3; ++axis) {
        if (std::abs(r.direction[axis]) < 1e-14) {
            if (r.origin[axis] < box.min[axis] || r.origin[axis] > box.max[axis])
                return false;
            continue;
        }
        double a = (box.min[axis] - r.origin[axis]) / r.direction[axis],
               b = (box.max[axis] - r.origin[axis]) / r.direction[axis];
        if (a > b)
            std::swap(a, b);
        start = std::max(start, a);
        stop = std::min(stop, b);
    }
    if (start > stop)
        return false;
    double speed = std::sqrt(r.direction.length2());
    double t = start, previousT = t, previous = field(r.at(t));
    for (int step = 0; step < 2048 && t <= stop; ++step) {
        Vec3 p = r.at(t);
        double distance = field(p);
        if (std::abs(distance) < 2e-5 || distance * previous < 0) {
            if (distance * previous < 0)
                for (int i = 0; i < 32; ++i) {
                    double mid = (previousT + t) * .5;
                    double v = field(r.at(mid));
                    if (v * previous > 0) {
                        previousT = mid;
                        previous = v;
                    } else
                        t = mid;
                }
            p = r.at(t);
            const double delta = 1e-4;
            Vec3 normal{field(p + Vec3{delta, 0, 0}) - field(p - Vec3{delta, 0, 0}),
                        field(p + Vec3{0, delta, 0}) - field(p - Vec3{0, delta, 0}),
                        field(p + Vec3{0, 0, delta}) - field(p - Vec3{0, 0, delta})};
            if (normal.length2() < 1e-20)
                return false;
            return finish(r, t, normal, lo, hi, hit);
        }
        previousT = t;
        previous = distance;
        double increment = type_ == "tanglecube" ? .008 : std::max(1e-5, std::abs(distance) * .65);
        t += increment / speed;
    }
    return false;
}

void registerImplicit(Factory &f) {
    for (const auto &name : {"torus", "tanglecube", "fractal"})
        f.shapes[name] = [](const Json &j, auto material) {
            return std::make_shared<Implicit>(j, std::move(material));
        };
}
} // namespace rt
