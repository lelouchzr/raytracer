#include "rt/engine.hpp"
#include "rt/math/Color.hpp"
#include "rt/render/BasicIntegrator.hpp"
#ifdef RT_BONUS
#include "rt/io/Hdr.hpp"
#endif
#include <chrono>
#include <fstream>
#include <iostream>

namespace rt {

std::shared_ptr<const IIntegrator> basicIntegrator() {
    return std::make_shared<BasicIntegrator>();
}

void writePpm(const Scene &s, const std::vector<Vec3> &pixels) {
#ifdef RT_BONUS
    if (std::filesystem::path(s.settings.output).extension() == ".pfm") {
        writePfm(s, pixels);
        return;
    }
#endif
    const std::filesystem::path output = s.settings.output;
    const auto temporary =
        output.string() + ".tmp." +
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    try {
        std::ofstream stream(temporary, std::ios::binary);
        if (!stream)
            throw std::runtime_error("cannot create output '" + output.string() + "'");
        stream << "P6\n" << s.settings.width << ' ' << s.settings.height << "\n255\n";
        for (Vec3 p : pixels)
            for (int i = 0; i < 3; ++i) {
                double c = p[i];
                if (!std::isfinite(c))
                    throw std::runtime_error("non-finite rendered color");
                // Gamma conversion is applied once, after all linear-light samples.
                unsigned char v =
                    static_cast<unsigned char>(255 * displayChannel(c, s.settings.colorSpace) + .5);
                stream.put(static_cast<char>(v));
            }
        stream.flush();
        if (!stream)
            throw std::runtime_error("failed to write output '" + output.string() + "'");
        stream.close();
        if (!stream)
            throw std::runtime_error("failed to close output file");
        std::filesystem::rename(temporary, output);
    } catch (...) {
        std::error_code ignored;
        std::filesystem::remove(temporary, ignored);
        throw;
    }
}

void render(const Scene &s) {
#ifdef RT_BONUS
    bonusRender(s);
#else
    std::vector<Vec3> pixels(static_cast<std::size_t>(s.settings.width) *
                             static_cast<std::size_t>(s.settings.height));
    unsigned long long rng = 1;
    const auto camera = s.camera.frame(s.settings);
    for (int y = 0; y < s.settings.height; ++y)
        for (int x = 0; x < s.settings.width; ++x)
            pixels[static_cast<std::size_t>(y) * static_cast<std::size_t>(s.settings.width) +
                   static_cast<std::size_t>(x)] =
                s.integrator->trace(s, camera.ray(x + .5, y + .5), 0, rng);
    writePpm(s, pixels);
#endif
}
} // namespace rt
