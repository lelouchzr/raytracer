#include "rt/lights/Environment.hpp"
#include "rt/Extensions.hpp"

namespace rt {
Environment::Environment(const Json &json, const std::filesystem::path &base)
    : image_(base / json.at("file").string()),
      intensity_(range(json.number("intensity", 1), 0, 10000, "environment intensity")),
      rotation_(range(json.number("rotation", 0), -36000, 36000, "environment rotation") * pi /
                180) {
    keys(json, {"file", "intensity", "rotation"});
    int width = image_.width(), height = image_.height();
    for (int y = 0; y < height; ++y) {
        double solidAngle =
            2 * pi / width * (std::cos(pi * y / height) - std::cos(pi * (y + 1) / height));
        for (int x = 0; x < width; ++x) {
            Vec3 color = image_.sample((x + .5) / width, (y + .5) / height);
            double luminance = .2126 * color.x + .7152 * color.y + .0722 * color.z;
            // A small uniform component gives every direction support even with bilinear filtering.
            double weight = luminance * solidAngle;
            total_ += weight;
            cdf_.push_back(total_);
            probabilities_.push_back(weight);
        }
    }
    double cumulative = 0;
    for (std::size_t i = 0; i < probabilities_.size(); ++i) {
        int y = static_cast<int>(i / static_cast<std::size_t>(width));
        double uniform =
            (std::cos(pi * y / height) - std::cos(pi * (y + 1) / height)) / (2 * width);
        probabilities_[i] =
            total_ > 0 ? .999 * probabilities_[i] / total_ + .001 * uniform : uniform;
        cumulative += probabilities_[i];
        cdf_[i] = cumulative;
    }
    for (double &value : probabilities_)
        value /= cumulative;
    for (double &value : cdf_)
        value /= cumulative;
    cdf_.back() = 1;
}

std::pair<int, int> Environment::pixel(Vec3 direction) const {
    direction = unit(direction);
    double phi = std::atan2(direction.z, direction.x) - rotation_;
    double u = phi / (2 * pi);
    u -= std::floor(u);
    double v = std::acos(std::clamp(direction.y, -1., 1.)) / pi;
    return {std::min(image_.width() - 1, static_cast<int>(u * image_.width())),
            std::min(image_.height() - 1, static_cast<int>(v * image_.height()))};
}

Vec3 Environment::environment(Vec3 direction) const {
    direction = unit(direction);
    double u = (std::atan2(direction.z, direction.x) - rotation_) / (2 * pi);
    double v = std::acos(std::clamp(direction.y, -1., 1.)) / pi;
    v = std::clamp(v, .5 / image_.height(), 1 - .5 / image_.height());
    return image_.sample(u, v) * intensity_;
}

LightSample Environment::sample(Vec3) const {
    Vec3 direction{0, 1, 0};
    return {direction, environment(direction) * (4 * pi), infinity, false, 1 / (4 * pi)};
}

LightSample Environment::sample(Vec3 point, unsigned long long &rng) const {
    auto it = std::upper_bound(cdf_.begin(), cdf_.end(), random01(rng));
    auto index = std::min(static_cast<std::size_t>(it - cdf_.begin()), cdf_.size() - 1);
    int x = static_cast<int>(index % static_cast<std::size_t>(image_.width()));
    int y = static_cast<int>(index / static_cast<std::size_t>(image_.width()));
    double phi = 2 * pi * (x + random01(rng)) / image_.width() + rotation_;
    double top = std::cos(pi * y / image_.height()),
           bottom = std::cos(pi * (y + 1) / image_.height());
    double cosine = bottom + random01(rng) * (top - bottom),
           sine = std::sqrt(std::max(0., 1 - cosine * cosine));
    Vec3 direction{sine * std::cos(phi), cosine, sine * std::sin(phi)};
    double density = directionPdf(point, direction);
    return {direction, environment(direction) / density, infinity, false, density};
}

double Environment::directionPdf(Vec3, Vec3 direction) const {
    auto [x, y] = pixel(direction);
    double area = 2 * pi / image_.width() *
                  (std::cos(pi * y / image_.height()) - std::cos(pi * (y + 1) / image_.height()));
    return probabilities_[static_cast<std::size_t>(y) * static_cast<std::size_t>(image_.width()) +
                          static_cast<std::size_t>(x)] /
           area;
}
} // namespace rt
