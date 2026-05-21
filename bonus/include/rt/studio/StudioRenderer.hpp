#pragma once
#include "rt/concurrency/JoiningThread.hpp"
#include "rt/engine.hpp"
#include "rt/render/RenderControl.hpp"
#include <atomic>
#include <chrono>

namespace rt {
struct StudioProgress {
    std::shared_ptr<RenderControl> control;
    Settings settings;
    unsigned long long generation = 0;
    int pass = 0;
    bool running = false;
    double seconds = 0;
    std::string error;
};

class StudioRenderer {
    mutable std::mutex mutex_;
    StudioProgress progress_;
    std::atomic<bool> stop_{false};
    JoiningThread worker_;
    std::filesystem::path directory_;
    std::chrono::steady_clock::time_point started_;
    void run(Json, unsigned long long, const std::filesystem::path &);

  public:
    StudioRenderer();
    ~StudioRenderer();
    StudioRenderer(const StudioRenderer &) = delete;
    StudioRenderer &operator=(const StudioRenderer &) = delete;
    void restart(const Json &, const std::filesystem::path &hdrOutput = {});
    void cancel();
    StudioProgress progress() const;
};
} // namespace rt
