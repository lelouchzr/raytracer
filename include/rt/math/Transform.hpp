#pragma once
#include "rt/math/Constants.hpp"
#include "rt/math/Vec3.hpp"

namespace rt {
struct Transform {
    Vec3 translation, rotation;
    Vec3 rotate(Vec3 v, bool inverse = false) const;

    Vec3 point(Vec3 v) const;

    Vec3 local(Vec3 v) const;
};
} // namespace rt
