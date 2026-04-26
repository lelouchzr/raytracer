#include "rt/math/Mat4.hpp"

namespace rt {
Mat4::Mat4() : values_{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1} {}

Mat4::Mat4(std::array<double, 16> values) : values_(values) {}

double Mat4::at(int row, int column) const {
    return values_[static_cast<std::size_t>(row * 4 + column)];
}

Mat4 Mat4::operator*(const Mat4 &b) const {
    std::array<double, 16> result{};
    for (int row = 0; row < 4; ++row)
        for (int col = 0; col < 4; ++col)
            for (int k = 0; k < 4; ++k)
                result[static_cast<std::size_t>(row * 4 + col)] += at(row, k) * b.at(k, col);
    return Mat4(result);
}

Vec3 Mat4::point(Vec3 p) const {
    return vector(p) + Vec3{at(0, 3), at(1, 3), at(2, 3)};
}

Vec3 Mat4::vector(Vec3 p) const {
    return {at(0, 0) * p.x + at(0, 1) * p.y + at(0, 2) * p.z,
            at(1, 0) * p.x + at(1, 1) * p.y + at(1, 2) * p.z,
            at(2, 0) * p.x + at(2, 1) * p.y + at(2, 2) * p.z};
}

Vec3 Mat4::normal(Vec3 p) const {
    return unit({at(0, 0) * p.x + at(1, 0) * p.y + at(2, 0) * p.z,
                 at(0, 1) * p.x + at(1, 1) * p.y + at(2, 1) * p.z,
                 at(0, 2) * p.x + at(1, 2) * p.y + at(2, 2) * p.z});
}

Mat4 Mat4::inverse() const {
    double matrix[4][8]{};
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j) {
            matrix[i][j] = at(i, j);
            matrix[i][j + 4] = i == j ? 1 : 0;
        }
    for (int col = 0; col < 4; ++col) {
        int pivot = col;
        for (int row = col + 1; row < 4; ++row)
            if (std::abs(matrix[row][col]) > std::abs(matrix[pivot][col]))
                pivot = row;
        if (std::abs(matrix[pivot][col]) < 1e-12)
            throw std::runtime_error("singular transformation matrix");
        for (int j = 0; j < 8; ++j)
            std::swap(matrix[col][j], matrix[pivot][j]);
        double divisor = matrix[col][col];
        for (double &v : matrix[col])
            v /= divisor;
        for (int row = 0; row < 4; ++row)
            if (row != col) {
                double factor = matrix[row][col];
                for (int j = 0; j < 8; ++j)
                    matrix[row][j] -= factor * matrix[col][j];
            }
    }
    std::array<double, 16> out{};
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            out[static_cast<std::size_t>(i * 4 + j)] = matrix[i][j + 4];
    return Mat4(out);
}

Mat4 Mat4::fromObject(const Json &j) {
    Vec3 scale = j.vector("scale", {1, 1, 1}), shear = j.vector("shear", {}),
         rotation = j.vector("rotate", {}), translation = j.vector("translate", {});
    for (int i = 0; i < 3; ++i)
        range(std::abs(scale[i]), 1e-4, 1e4, "absolute object scale");
    Mat4 s({scale.x, shear.x, shear.y, 0, 0, scale.y, shear.z, 0, 0, 0, scale.z, 0, 0, 0, 0, 1});
    Transform t{translation, rotation};
    Vec3 x = t.rotate({1, 0, 0}), y = t.rotate({0, 1, 0}), z = t.rotate({0, 0, 1});
    Mat4 transform({x.x, y.x, z.x, translation.x, x.y, y.y, z.y, translation.y, x.z, y.z, z.z,
                    translation.z, 0, 0, 0, 1});
    if (j.has("matrix")) {
        const auto &a = j.at("matrix").array();
        if (a.size() != 16)
            throw std::runtime_error("matrix requires 16 row-major numbers");
        std::array<double, 16> v{};
        for (std::size_t i = 0; i < 16; ++i)
            v[i] = range(a[i].number(), -1e9, 1e9, "matrix element");
        if (v[12] != 0 || v[13] != 0 || v[14] != 0 || v[15] != 1)
            throw std::runtime_error("matrix must be affine [0,0,0,1] last row");
        transform = transform * Mat4(v);
    }
    return transform * s;
}
} // namespace rt
