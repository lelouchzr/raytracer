#pragma once
#include "rt/math.hpp"
#include <atomic>
#include <mutex>
#include <vector>

namespace rt {
class RenderControl {
    mutable std::mutex mutex_;
    std::vector<Vec3> pixels_;
    std::atomic<bool> cancelled_{false};
    std::atomic<int> rows_{0};
    std::atomic<unsigned long long> samples_{0};
    int width_;

  public:
    RenderControl(int width, int height);
    void cancel();
    bool cancelled() const;
    void publish(int row, const std::vector<Vec3> &, unsigned long long samples);
    std::vector<Vec3> snapshot() const;
    int rows() const;
    unsigned long long samples() const;
};
} // namespace rt
