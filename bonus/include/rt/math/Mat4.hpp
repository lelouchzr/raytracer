#pragma once
#include "rt/json.hpp"

namespace rt {
class Mat4 {
    std::array<double, 16> values_;

  public:
    Mat4();
    explicit Mat4(std::array<double, 16>);
    double at(int row, int column) const;
    Mat4 operator*(const Mat4 &) const;
    Vec3 point(Vec3) const;
    Vec3 vector(Vec3) const;
    Vec3 normal(Vec3) const;
    Mat4 inverse() const;
    static Mat4 fromObject(const Json &);
};
} // namespace rt
