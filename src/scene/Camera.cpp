#include "rt/engine.hpp"
#include <fstream>
#include <sstream>

namespace rt {
void Camera::validate() const {
    range(fov, 1, 175, "camera fov");
    unit(cross(unit(target - position), up));
}

CameraFrame Camera::frame(const Settings &s) const {
    Vec3 forward = unit(target - position), right = unit(cross(forward, up)),
         vertical = cross(right, forward);
    double scale = std::tan(fov * pi / 360), aspect = double(s.width) / s.height;
    return {position, forward - right * (aspect * scale) + vertical * scale,
            right * (2 * aspect * scale / s.width), vertical * (-2 * scale / s.height)};
}

Ray Camera::ray(double x, double y, const Settings &s) const {
    return frame(s).ray(x, y);
}

} // namespace rt
