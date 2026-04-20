#pragma once
#include "rt/math/Constants.hpp"

namespace rt {
struct Vec3 {
    double x = 0, y = 0, z = 0;
    double operator[](int i) const;

    Vec3 operator-() const;

    Vec3 operator+(Vec3 b) const;

    Vec3 operator-(Vec3 b) const;

    Vec3 operator*(double s) const;

    Vec3 operator*(Vec3 b) const;

    Vec3 operator/(double s) const;

    Vec3 &operator+=(Vec3 b);

    double length2() const;
};
} // namespace rt
