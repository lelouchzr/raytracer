#include "rt/render/RenderControl.hpp"

namespace rt {
RenderControl::RenderControl(int width, int height)
    : pixels_(static_cast<std::size_t>(width) * static_cast<std::size_t>(height)), width_(width),
      completed_(static_cast<std::size_t>(height), 0) {}

void RenderControl::cancel() {
    cancelled_ = true;
}

bool RenderControl::cancelled() const {
    return cancelled_;
}

void RenderControl::publish(int row, const std::vector<Vec3> &pixels, unsigned long long samples) {
    std::lock_guard lock(mutex_);
    std::copy(pixels.begin(), pixels.end(),
              pixels_.begin() + static_cast<std::ptrdiff_t>(row) * width_);
    completed_[static_cast<std::size_t>(row)] = 1;
    samples_ += samples;
    ++rows_;
}

std::vector<Vec3> RenderControl::snapshot() const {
    std::lock_guard lock(mutex_);
    return pixels_;
}

std::vector<unsigned char> RenderControl::completedRows() const {
    std::lock_guard lock(mutex_);
    return completed_;
}

int RenderControl::rows() const {
    return rows_;
}

unsigned long long RenderControl::samples() const {
    return samples_;
}
} // namespace rt
