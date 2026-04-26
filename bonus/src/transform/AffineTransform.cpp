#include "rt/transform/AffineTransform.hpp"

namespace rt {
AffineTransform::AffineTransform(Primitive child, const Mat4 &matrix, bool identifyBoundary)
    : child_(std::move(child)), matrix_(matrix), inverse_(matrix.inverse()),
      identifyBoundary_(identifyBoundary) {}

bool AffineTransform::intersect(const Ray &ray, double lo, double hi, Hit &hit) const {
    if (!child_->intersect({inverse_.point(ray.origin), inverse_.vector(ray.direction)}, lo, hi,
                           hit))
        return false;
    hit.point = ray.at(hit.t);
    hit.normal = inverse_.normal(hit.normal);
    hit.geometric = inverse_.normal(hit.geometric);
    hit.tangent = unit(matrix_.vector(hit.tangent));
    hit.bitangent = unit(matrix_.vector(hit.bitangent));
    if (identifyBoundary_)
        hit.boundary = this;
    return true;
}

Bounds AffineTransform::bounds() const {
    auto b = child_->bounds();
    if (!b.finite())
        return b;
    Bounds result;
    for (int i = 0; i < 8; ++i)
        result.include(matrix_.point({(i & 1) ? b.max.x : b.min.x, (i & 2) ? b.max.y : b.min.y,
                                      (i & 4) ? b.max.z : b.min.z}));
    return result;
}

Primitive applyAffine(Primitive primitive, const Json &j) {
    return std::make_shared<AffineTransform>(std::move(primitive), Mat4::fromObject(j));
}
} // namespace rt
