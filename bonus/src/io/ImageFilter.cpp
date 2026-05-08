#include "rt/io/Image.hpp"

namespace rt {
void Image::buildMips() {
    mips_.push_back({width_, height_, std::move(pixels_)});
    while (mips_.back().width > 1 || mips_.back().height > 1) {
        const auto &source = mips_.back();
        int width = std::max(1, (source.width + 1) / 2),
            height = std::max(1, (source.height + 1) / 2);
        MipLevel next{
            width, height,
            std::vector<Vec3>(static_cast<std::size_t>(width) * static_cast<std::size_t>(height))};
        for (int y = 0; y < height; ++y)
            for (int x = 0; x < width; ++x) {
                Vec3 sum;
                for (int dy = 0; dy < 2; ++dy)
                    for (int dx = 0; dx < 2; ++dx) {
                        int sx = std::min(x * 2 + dx, source.width - 1),
                            sy = std::min(y * 2 + dy, source.height - 1);
                        sum += source.pixels[static_cast<std::size_t>(sy) *
                                                 static_cast<std::size_t>(source.width) +
                                             static_cast<std::size_t>(sx)];
                    }
                next.pixels[static_cast<std::size_t>(y) * static_cast<std::size_t>(width) +
                            static_cast<std::size_t>(x)] = sum * .25;
            }
        mips_.push_back(std::move(next));
    }
}

Vec3 Image::filtered(double u, double v, std::size_t level) const {
    const auto &mip = mips_[level];
    double x = (u - std::floor(u)) * mip.width - .5, y = (v - std::floor(v)) * mip.height - .5;
    int ix = static_cast<int>(std::floor(x)), iy = static_cast<int>(std::floor(y));
    double tx = x - ix, ty = y - iy;
    auto texel = [&](int a, int b) {
        a = (a % mip.width + mip.width) % mip.width;
        b = (b % mip.height + mip.height) % mip.height;
        return mip.pixels[static_cast<std::size_t>(b) * static_cast<std::size_t>(mip.width) +
                          static_cast<std::size_t>(a)];
    };
    return (texel(ix, iy) * (1 - tx) + texel(ix + 1, iy) * tx) * (1 - ty) +
           (texel(ix, iy + 1) * (1 - tx) + texel(ix + 1, iy + 1) * tx) * ty;
}

Vec3 Image::sample(double u, double v, double footprint) const {
    if (!std::isfinite(u) || !std::isfinite(v) || !std::isfinite(footprint))
        throw std::runtime_error("non-finite texture coordinates/footprint");
    double lod = std::clamp(std::log2(std::max(1., footprint * std::max(width_, height_))), 0.,
                            double(mips_.size() - 1));
    auto level = static_cast<std::size_t>(lod);
    double fraction = lod - double(level);
    return filtered(u, v, level) * (1 - fraction) +
           filtered(u, v, std::min(level + 1, mips_.size() - 1)) * fraction;
}
} // namespace rt
