#include "rt/engine.hpp"
#include <bit>
#include <cstdint>
#include <iostream>
#ifdef RT_BONUS
#include "rt/math/Polynomial.hpp"
#include "rt/render/MediumStack.hpp"
#endif

namespace {
void require(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(message);
}

bool referenceSlabs(const rt::Bounds &bounds, rt::Ray ray, double lo, double hi) {
    // Preserve the pre-optimization contract as a differential oracle.
    for (int axis = 0; axis < 3; ++axis) {
        if (std::abs(ray.direction[axis]) < 1e-14) {
            if (ray.origin[axis] < bounds.min[axis] || ray.origin[axis] > bounds.max[axis])
                return false;
            continue;
        }
        double a = (bounds.min[axis] - ray.origin[axis]) / ray.direction[axis];
        double b = (bounds.max[axis] - ray.origin[axis]) / ray.direction[axis];
        if (a > b)
            std::swap(a, b);
        lo = std::max(lo, a);
        hi = std::min(hi, b);
        if (hi < lo)
            return false;
    }
    return true;
}

unsigned next(unsigned &state) {
    state = state * 1664525u + 1013904223u;
    return state;
}

void slabs() {
    unsigned state = 1234567;
    auto coordinate = [&]() { return (static_cast<double>(next(state) % 2049) - 1024) / 64; };
    for (int index = 0; index < 5000; ++index) {
        rt::Vec3 lower{coordinate(), coordinate(), coordinate()};
        rt::Bounds bounds{lower, lower + rt::Vec3{std::abs(coordinate()), std::abs(coordinate()),
                                                  std::abs(coordinate())}};
        rt::Ray ray{{coordinate(), coordinate(), coordinate()},
                    {coordinate(), coordinate(), coordinate()}};
        if (index % 4 == 0)
            ray.direction.x = 0;
        if (index % 7 == 0)
            ray.direction.y = -0.;
        if (index % 11 == 0)
            ray.direction.z = 1e-14;
        if (index % 13 == 0)
            ray.direction.z = 1e-15;
        double lo = index % 5 == 0 ? 2 : 0, hi = index % 9 == 0 ? lo : 100;
        require(bounds.hit(ray, lo, hi) == referenceSlabs(bounds, ray, lo, hi),
                "slab differential mismatch");
    }
    const rt::Bounds unit{{-1, -1, -1}, {1, 1, 1}};
    require(unit.hit({{3, 0, 0}, {-1, 0, 0}}, 2, 2), "closed slab interval lost tangent");
    require(unit.hit({{1, 0, 0}, {-0., 1, 0}}, 0, 1), "parallel boundary/signed zero changed");
}

#ifdef RT_BONUS
void quartics() {
    auto compare = [](const std::array<double, 5> &coefficients) {
        const auto reference =
            rt::Polynomial::roots(std::vector<double>(coefficients.begin(), coefficients.end()));
        rt::Polynomial::RootBuffer actual{};
        auto count = rt::Polynomial::roots(coefficients, actual);
        require(count == reference.size(), "quartic root count changed");
        for (std::size_t index = 0; index < count; ++index)
            require(std::bit_cast<std::uint64_t>(actual[index]) ==
                        std::bit_cast<std::uint64_t>(reference[index]),
                    "quartic root bits changed");
    };
    for (const auto &coefficients : std::array<std::array<double, 5>, 8>{{{0, 0, 0, 0, 0},
                                                                          {1, 2, 0, 0, 0},
                                                                          {-1e-13, 0, 1, 0, 0},
                                                                          {0, 0, 0, 0, 1},
                                                                          {1, -4, 6, -4, 1},
                                                                          {-6, 11, -6, 1, 1e-19},
                                                                          {1e20, -1e12, 3, -2, 1},
                                                                          {-0., -0., 1, 0, 0}}})
        compare(coefficients);
    rt::Polynomial::RootBuffer toleranceCandidates{};
    require(rt::Polynomial::roots({-1e-13, 0, 1, 0, 0}, toleranceCandidates) == 3,
            "near-zero critical candidate was dropped");
    unsigned state = 891011;
    for (int trial = 0; trial < 800; ++trial) {
        std::array<double, 5> coefficients{};
        for (double &coefficient : coefficients)
            coefficient = (static_cast<double>(next(state) % 2001) - 1000) / 128;
        compare(coefficients);
    }
    const auto fifth = rt::Polynomial::roots(std::vector<double>{0, -1, 0, 0, 0, 1});
    require(fifth.size() == 3, "generic degree-five solver changed");
}

void media() {
    rt::MediumStack stack;
    int a = 0, b = 0, c = 0, absent = 0;
    auto compare = [&](const rt::MediumStack &current, rt::Hit hit, double ior,
                       bool empty = false) {
        auto indices = current.indices(hit, ior);
        double incoming = current.current();
        if (!hit.front && empty)
            incoming = ior;
        require(indices.first == incoming && indices.second == current.cross(hit, ior).current(),
                "medium indices differ from state transition");
    };
    rt::Hit hit;
    hit.boundary = &a;
    hit.front = false;
    compare(stack, hit, 1.5, true);
    hit.front = true;
    compare(stack, hit, 1.5, true);
    stack = stack.cross(hit, 1.5);
    hit.boundary = &b;
    stack = stack.cross(hit, 1.33);
    hit.boundary = &a;
    stack = stack.cross(hit, 1.7); // Repeated boundary: remove most recent.
    hit.boundary = &c;
    stack = stack.cross(hit, 1.2);
    hit.front = false;
    for (const void *boundary : {&a, &b, &c, &absent}) {
        hit.boundary = boundary;
        compare(stack, hit, 1.7);
    }
    require(stack.current() == 1.2, "indices mutated original medium stack");
    rt::MediumStack unity;
    hit.front = true;
    hit.boundary = &a;
    unity = unity.cross(hit, 1);
    hit.front = false;
    compare(unity, hit, 1.7);
}

void boxTies() {
    rt::Factory factory;
    rt::registerBonus(factory);
    auto material = std::make_shared<rt::FlatMaterial>(rt::Vec3{1, 1, 1});
    auto box = factory.primitive(rt::Json::parse(R"({"type":"box","size":[2,2,2]})"), material);
    rt::Hit hit;
    require(box->intersect({{3, 3, 0}, {-1, -1, 0}}, 0, 2, hit), "box closed interval changed");
    require(hit.t == 2 && hit.normal.x == 1 && hit.normal.y == 0, "box corner axis tie changed");
    require(box->intersect({{0, 0, 0}, {1, 1, 0}}, 0, 1, hit), "inside box exit changed");
    require(!hit.front && hit.normal.x == -1 && hit.normal.y == 0, "box exit tie changed");
}
#endif
} // namespace

int main() {
    try {
        slabs();
#ifdef RT_BONUS
        quartics();
        media();
        boxTies();
#endif
        std::cout << "Performance regressions preserve slab, root-bit, medium and tie contracts\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
