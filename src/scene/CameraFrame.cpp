#include "rt/engine.hpp"

namespace rt {
Ray CameraFrame::ray(double x, double y) const {
    return {position, unit(corner + stepX * x + stepY * y)};
}
} // namespace rt
