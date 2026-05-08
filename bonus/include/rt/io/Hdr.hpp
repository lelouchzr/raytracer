#pragma once
#include "rt/math.hpp"
#include <filesystem>
#include <span>
#include <vector>

namespace rt {
struct Scene;
bool decodeRadiance(std::span<const unsigned char>, int &, int &, std::vector<Vec3> &);
void writePfm(const Scene &, const std::vector<Vec3> &);
} // namespace rt
