#include "rt/plugins/LeasePrimitive.hpp"

namespace rt {
LeasePrimitive::LeasePrimitive(std::shared_ptr<SharedLibrary> library, Primitive primitive)
    : library_(std::move(library)), primitive_(std::move(primitive)) {
    if (!primitive_)
        throw std::runtime_error("plugin returned a null primitive");
}

bool LeasePrimitive::intersect(const Ray &r, double lo, double hi, Hit &h) const {
    return primitive_->intersect(r, lo, hi, h);
}

Bounds LeasePrimitive::bounds() const {
    return primitive_->bounds();
}
} // namespace rt
