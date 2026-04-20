#include "rt/math.hpp"

namespace rt {
double Vec3::operator[](int i) const {
    return i == 0 ? x : i == 1 ? y : z;
}

Vec3 Vec3::operator-() const {
    return {-x, -y, -z};
}

Vec3 Vec3::operator+(Vec3 b) const {
    return {x + b.x, y + b.y, z + b.z};
}

Vec3 Vec3::operator-(Vec3 b) const {
    return {x - b.x, y - b.y, z - b.z};
}

Vec3 Vec3::operator*(double s) const {
    return {x * s, y * s, z * s};
}

Vec3 Vec3::operator*(Vec3 b) const {
    return {x * b.x, y * b.y, z * b.z};
}

Vec3 Vec3::operator/(double s) const {
    return *this * (1 / s);
}

Vec3 &Vec3::operator+=(Vec3 b) {
    *this = *this + b;
    return *this;
}

double Vec3::length2() const {
    return x * x + y * y + z * z;
}
} // namespace rt
