#include "rt/materials/Perlin.hpp"
#include <numeric>

namespace rt {
Perlin::Perlin() {
    std::array<int, 256> p{};
    std::iota(p.begin(), p.end(), 0);
    unsigned seed = 0x12345678;
    for (int i = 255; i > 0; --i) {
        seed = 1664525 * seed + 1013904223;
        int j = static_cast<int>(seed % static_cast<unsigned>(i + 1));
        std::swap(p[static_cast<std::size_t>(i)], p[static_cast<std::size_t>(j)]);
    }
    for (std::size_t i = 0; i < 512; ++i)
        permutation_[i] = p[i & 255];
}

double Perlin::gradient(int hash, double x, double y, double z) {
    int h = hash & 15;
    double u = h < 8 ? x : y, v = h < 4 ? y : (h == 12 || h == 14 ? x : z);
    return ((h & 1) ? -u : u) + ((h & 2) ? -v : v);
}

double Perlin::noise(Vec3 p) const {
    p = {std::fmod(p.x, 256), std::fmod(p.y, 256), std::fmod(p.z, 256)};
    int x = static_cast<int>(std::floor(p.x)) & 255, y = static_cast<int>(std::floor(p.y)) & 255,
        z = static_cast<int>(std::floor(p.z)) & 255;
    p = {p.x - std::floor(p.x), p.y - std::floor(p.y), p.z - std::floor(p.z)};
    auto fade = [](double t) { return t * t * t * (t * (t * 6 - 15) + 10); };
    auto lerp = [](double a, double b, double t) { return a + (b - a) * t; };
    auto perm = [&](int i) { return permutation_[static_cast<std::size_t>(i)]; };
    double u = fade(p.x), v = fade(p.y), w = fade(p.z);
    int a = perm(x) + y, aa = perm(a) + z, ab = perm(a + 1) + z, b = perm(x + 1) + y,
        ba = perm(b) + z, bb = perm(b + 1) + z;
    return lerp(
        lerp(lerp(gradient(perm(aa), p.x, p.y, p.z), gradient(perm(ba), p.x - 1, p.y, p.z), u),
             lerp(gradient(perm(ab), p.x, p.y - 1, p.z), gradient(perm(bb), p.x - 1, p.y - 1, p.z),
                  u),
             v),
        lerp(lerp(gradient(perm(aa + 1), p.x, p.y, p.z - 1),
                  gradient(perm(ba + 1), p.x - 1, p.y, p.z - 1), u),
             lerp(gradient(perm(ab + 1), p.x, p.y - 1, p.z - 1),
                  gradient(perm(bb + 1), p.x - 1, p.y - 1, p.z - 1), u),
             v),
        w);
}
} // namespace rt
