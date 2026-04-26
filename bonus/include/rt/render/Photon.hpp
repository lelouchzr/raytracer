#pragma once
#include "rt/math.hpp"

namespace rt {
struct Photon {
    Vec3 position, direction, power;
    const void *receiver = nullptr;
};
} // namespace rt
