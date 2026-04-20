#include "rt/math.hpp"

namespace rt {
Vec3 Ray::at(double t) const {
    return origin + t * direction;
}
} // namespace rt
