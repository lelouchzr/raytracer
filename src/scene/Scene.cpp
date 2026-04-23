#include "rt/engine.hpp"

namespace rt {
bool Scene::intersect(Ray r, double lo, double hi, Hit &h) const {
    if (acceleration)
        return acceleration->intersect(r, lo, hi, h);
    bool found = false;
    for (const auto &o : objects)
        if (o->intersect(r, lo, hi, h)) {
            hi = h.t;
            found = true;
        }
    return found;
}

} // namespace rt
