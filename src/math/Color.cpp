#include "rt/math/Color.hpp"

namespace rt {
double srgbDecode(double value) {
    return value <= .04045 ? value / 12.92 : std::pow((value + .055) / 1.055, 2.4);
}

double srgbEncode(double value) {
    return value <= .0031308 ? value * 12.92 : 1.055 * std::pow(value, 1 / 2.4) - .055;
}

double displayChannel(double value, std::string_view colorSpace) {
    value = std::clamp(value, 0., 1.);
    return colorSpace == "srgb" ? srgbEncode(value) : std::sqrt(value);
}

Vec3 toneMap(Vec3 color, std::string_view mode) {
    if (mode == "none")
        return color;
    // Uniform scaling preserves linear RGB ratios, unlike per-channel compression.
    if (mode == "neutral")
        return color / (1 + std::max({color.x, color.y, color.z}));
    return {color.x / (1 + color.x), color.y / (1 + color.y), color.z / (1 + color.z)};
}
} // namespace rt
