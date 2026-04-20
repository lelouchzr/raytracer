#pragma once
#include "rt/render/Settings.hpp"
#include "rt/scene/CameraFrame.hpp"

namespace rt {
struct Camera {
    Vec3 position{0, 2, 7}, target{0, 1, 0}, up{0, 1, 0};
    double fov = 45;
    CameraFrame frame(const Settings &) const;
    Ray ray(double x, double y, const Settings &) const;
    void validate() const;
};
} // namespace rt
