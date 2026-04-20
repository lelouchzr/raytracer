#pragma once
#include "rt/math/Bounds.hpp"
#include "rt/math/Ray.hpp"
#include "rt/math/Transform.hpp"
#include "rt/math/Vec3.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace rt {

Vec3 operator*(double s, Vec3 v);
double dot(Vec3 a, Vec3 b);
Vec3 cross(Vec3 a, Vec3 b);
Vec3 unit(Vec3 v);
Vec3 reflect(Vec3 v, Vec3 n);

// Rigid transformation: Euler X, then Y, then Z, followed by translation.

} // namespace rt
