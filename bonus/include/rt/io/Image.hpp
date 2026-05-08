#pragma once
#include "rt/math.hpp"
#include <filesystem>
#include <vector>

namespace rt {
struct MipLevel {
    int width, height;
    std::vector<Vec3> pixels;
};

class Image {
    int width_ = 0, height_ = 0;
    std::vector<Vec3> pixels_;
    std::vector<MipLevel> mips_;
    void buildMips();
    Vec3 filtered(double, double, std::size_t) const;

  public:
    explicit Image(const std::filesystem::path &, bool linearize = true);
    int width() const;
    int height() const;
    Vec3 sample(double u, double v, double footprint = 0) const;
};
} // namespace rt
