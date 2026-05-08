#pragma once
#include "rt/math/Vec3.hpp"
#include <string_view>

namespace rt {
double srgbDecode(double);
double srgbEncode(double);
double displayChannel(double, std::string_view colorSpace);
Vec3 toneMap(Vec3, std::string_view mode);
} // namespace rt
