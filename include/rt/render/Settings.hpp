#pragma once
#include <string>

namespace rt {
struct Settings {
    int width = 640, height = 360, samples = 1, depth = 6, threads = 1, ao = 0;
    double exposure = 1;
    std::string output = "output.ppm", integrator = "whitted";
    int photons = 0, minSamples = 4;
    double photonRadius = .3, adaptive = 0;
};
} // namespace rt
