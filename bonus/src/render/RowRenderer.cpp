#include "rt/render/RowRenderer.hpp"
#include "rt/Extensions.hpp"
#include "rt/concurrency/JoiningThread.hpp"
#include "rt/math/Color.hpp"
#include <atomic>
#include <numeric>
#include <thread>

namespace rt {
RowRenderer::RowRenderer(const Scene &s) : scene_(s), camera_(s.camera.frame(s.settings)) {}

void RowRenderer::row(int y, RenderControl &control) const {
    const auto &s = scene_.settings;
    std::vector<Vec3> row(static_cast<std::size_t>(s.width));
    std::vector<int> permutation(static_cast<std::size_t>(s.samples));
    unsigned long long total = 0;
    for (int x = 0; x < s.width && !control.cancelled(); ++x) {
        unsigned long long rng =
            (1 + static_cast<unsigned long long>(y) * static_cast<unsigned>(s.width) +
             static_cast<unsigned>(x)) *
            0x9e3779b97f4a7c15ULL;
        if (!rng)
            rng = 1;
        std::iota(permutation.begin(), permutation.end(), 0);
        for (int i = s.samples - 1; i > 0; --i) {
            int j = static_cast<int>(random01(rng) * (i + 1));
            std::swap(permutation[static_cast<std::size_t>(i)],
                      permutation[static_cast<std::size_t>(j)]);
        }
        Vec3 sum, mean, m2;
        int count = 0;
        for (int i = 0; i < s.samples && !control.cancelled(); ++i) {
            double dx = s.samples == 1 ? .5 : (i + random01(rng)) / s.samples,
                   dy =
                       s.samples == 1
                           ? .5
                           : (permutation[static_cast<std::size_t>(i)] + random01(rng)) / s.samples;
            // Adaptive samples are independent uniform samples: early stopping must not bias
            // the pixel footprint toward the first Latin-hypercube strata.
            if (s.adaptive > 0) {
                dx = random01(rng);
                dy = random01(rng);
            }
            Ray ray = camera_.ray(x + dx, y + dy);
            ray.coneSpread =
                std::max(std::sqrt(camera_.stepX.length2()), std::sqrt(camera_.stepY.length2()));
            Vec3 value = scene_.integrator->trace(scene_, ray, 0, rng);
            sum += value;
            ++count;
            Vec3 delta = value - mean;
            mean += delta / count;
            m2 += delta * (value - mean);
            if (s.adaptive > 0 && count >= s.minSamples && count % 4 == 0) {
                Vec3 variance = m2 / (count - 1);
                double error = std::sqrt(std::max({variance.x, variance.y, variance.z}) / count);
                if (error <= s.adaptive)
                    break;
            }
        }
        if (count == 0)
            break;
        total += static_cast<unsigned>(count);
        bool hdr = std::filesystem::path(s.output).extension() == ".pfm";
        Vec3 c = sum * ((hdr ? 1 : s.exposure) / count);
        row[static_cast<std::size_t>(x)] = hdr ? c : toneMap(c, s.toneMap);
    }
    if (!control.cancelled())
        control.publish(y, row, total);
}

void renderImage(const Scene &s, RenderControl &control, int begin, int end) {
    if (end < 0)
        end = s.settings.height;
    if (begin < 0 || end > s.settings.height || begin > end)
        throw std::runtime_error("invalid render row range");
    RowRenderer renderer(s);
    std::atomic<int> next{begin};
    std::exception_ptr failure;
    std::mutex mutex;
    auto worker = [&]() {
        try {
            while (!control.cancelled()) {
                int y = next++;
                if (y >= end)
                    break;
                renderer.row(y, control);
            }
        } catch (...) {
            std::lock_guard lock(mutex);
            if (!failure)
                failure = std::current_exception();
            control.cancel();
        }
    };
    unsigned threads = s.settings.threads ? static_cast<unsigned>(s.settings.threads)
                                          : std::max(1u, std::thread::hardware_concurrency());
    threads = std::min(threads, static_cast<unsigned>(std::max(1, end - begin)));
    std::vector<JoiningThread> workers;
    for (unsigned i = 0; i < threads; ++i)
        workers.emplace_back(worker);
    workers.clear();
    if (failure)
        std::rethrow_exception(failure);
}
} // namespace rt
