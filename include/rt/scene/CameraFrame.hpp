#pragma once
#include "rt/math.hpp"

namespace rt {
struct CameraFrame {
    Vec3 position, corner, stepX, stepY;
    Ray ray(double x, double y) const;
};
} // namespace rt
