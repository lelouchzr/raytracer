#include "rt/Extensions.hpp"
#include "rt/acceleration/Composite.hpp"

namespace rt {
namespace {

// Composite: leaves and internal BVH nodes implement the same IPrimitive contract.

Primitive node(std::vector<Primitive> objects) {
    if (objects.size() <= 4)
        return std::make_shared<Composite>(std::move(objects));
    Bounds bounds;
    for (const auto &o : objects)
        bounds.include(o->bounds());
    Vec3 size = bounds.max - bounds.min;
    int axis = size.x > size.y ? (size.x > size.z ? 0 : 2) : (size.y > size.z ? 1 : 2);
    auto mid = objects.begin() + static_cast<std::ptrdiff_t>(objects.size() / 2);
    std::nth_element(objects.begin(), mid, objects.end(), [axis](const auto &a, const auto &b) {
        auto x = a->bounds(), y = b->bounds();
        return x.min[axis] + x.max[axis] < y.min[axis] + y.max[axis];
    });
    return std::make_shared<Composite>(
        std::vector<Primitive>{node({objects.begin(), mid}), node({mid, objects.end()})});
}
} // namespace

Primitive buildBvh(std::vector<Primitive> objects) {
    std::vector<Primitive> finite, root;
    for (const auto &o : objects)
        (o->bounds().finite() ? finite : root).push_back(o);
    if (!finite.empty())
        root.push_back(node(std::move(finite)));
    return std::make_shared<Composite>(std::move(root));
}

} // namespace rt
