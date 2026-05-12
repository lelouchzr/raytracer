#include "rt/studio/StudioRenderer.hpp"
#include "rt/render/AdvancedIntegrator.hpp"
#include "rt/render/RowRenderer.hpp"
#include "rt/studio/StudioDocument.hpp"
#include <unistd.h>

namespace rt {
StudioRenderer::StudioRenderer() {
    directory_ = std::filesystem::temp_directory_path() /
                 ("raytracer-native-" + std::to_string(getpid()) + "-" +
                  std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directory(directory_);
}

StudioRenderer::~StudioRenderer() {
    cancel();
    std::error_code error;
    std::filesystem::remove_all(directory_, error);
}

void StudioRenderer::cancel() {
    worker_.request_stop();
    {
        std::lock_guard lock(mutex_);
        if (progress_.control)
            progress_.control->cancel();
    }
    if (worker_.joinable())
        worker_.join();
    std::lock_guard lock(mutex_);
    progress_.running = false;
}

void StudioRenderer::restart(const Json &json, const std::filesystem::path &hdrOutput) {
    cancel();
    unsigned long long generation;
    {
        std::lock_guard lock(mutex_);
        generation = ++progress_.generation;
        started_ = std::chrono::steady_clock::now();
        progress_.error.clear();
        progress_.running = true;
        progress_.pass = 0;
    }
    worker_ = std::jthread([this, json, generation, hdrOutput](std::stop_token stop) {
        run(json, generation, hdrOutput, stop);
    });
}

StudioProgress StudioRenderer::progress() const {
    std::lock_guard lock(mutex_);
    auto result = progress_;
    if (result.running)
        result.seconds =
            std::chrono::duration<double>(std::chrono::steady_clock::now() - started_).count();
    return result;
}

void StudioRenderer::run(Json json, unsigned long long generation,
                         const std::filesystem::path &hdrOutput, std::stop_token stop) {
    auto start = std::chrono::steady_clock::now();
    try {
        auto file = directory_ / "scene.json";
        auto &root = std::get<Json::Object>(json.value);
        if (!root.count("render"))
            root["render"] = Json(Json::Object{});
        std::get<Json::Object>(root.at("render").value)["output"] =
            Json((directory_ / "preview.ppm").string());
        StudioDocument::write(json, file);
        auto base = SceneBuilder(false, true).build(file);
        int width = base.settings.width, height = base.settings.height,
            samples = base.settings.samples;
        std::vector<std::array<int, 3>> stages;
        for (auto [limit, budget] :
             {std::pair{320, 1}, std::pair{640, 4}, std::pair{width, samples}}) {
            double factor = std::min(1., double(limit) / width);
            std::array<int, 3> stage{std::max(1, static_cast<int>(width * factor)),
                                     std::max(1, static_cast<int>(height * factor)),
                                     std::min(samples, budget)};
            if (stages.empty() || stages.back() != stage)
                stages.push_back(stage);
        }
        int pass = 0;
        for (auto stage : stages) {
            if (stop.stop_requested())
                break;
            auto scene = base;
            scene.settings.width = stage[0];
            scene.settings.height = stage[1];
            scene.settings.samples = stage[2];
            if (!hdrOutput.empty() && stage == stages.back())
                scene.settings.output = hdrOutput.string();
            auto control = std::make_shared<RenderControl>(stage[0], stage[1]);
            {
                std::lock_guard lock(mutex_);
                if (stop.stop_requested()) {
                    control->cancel();
                    break;
                }
                progress_.control = control;
                progress_.settings = scene.settings;
                progress_.generation = generation;
                progress_.pass = ++pass;
            }
            if (stage != stages.back())
                scene.settings.photons = std::min(scene.settings.photons, pass == 1 ? 2000 : 10000);
            if (scene.settings.integrator == "photon")
                scene.integrator = std::make_shared<AdvancedIntegrator>(
                    scene, std::make_shared<PhotonMap>(scene, control.get()));
            renderImage(scene, *control);
            if (control->cancelled())
                break;
            if (!hdrOutput.empty() && stage == stages.back())
                writePpm(scene, control->snapshot());
        }
    } catch (const std::exception &error) {
        std::lock_guard lock(mutex_);
        progress_.error = error.what();
    } catch (...) {
        std::lock_guard lock(mutex_);
        progress_.error = "unexpected render failure";
    }
    std::lock_guard lock(mutex_);
    progress_.running = false;
    progress_.seconds =
        std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
}
} // namespace rt
