#pragma once
#include "rt/math.hpp"
#include <filesystem>
#include <vector>

namespace rt {
class Image {
    int width_ = 0, height_ = 0;
    std::vector<Vec3> pixels_;

  public:
    explicit Image(const std::filesystem::path &, bool linearize = true);
    int width() const;
    int height() const;
    Vec3 sample(double u, double v) const;
};
} // namespace rt
