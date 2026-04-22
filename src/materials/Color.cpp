#include "rt/engine.hpp"

namespace rt {
Vec3 colorValue(Vec3 c) {
    range(c.x, 0, 1, "color red");
    range(c.y, 0, 1, "color green");
    range(c.z, 0, 1, "color blue");
    return c;
}

} // namespace rt
