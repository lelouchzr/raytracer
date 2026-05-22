#include "rt/studio/StudioTransforms.hpp"
#include <cmath>
#include <stdexcept>
#include <utility>

namespace rt {
void StudioTransforms::normalize(Json &object) {
    auto &fields = std::get<Json::Object>(object.value);
    for (const auto &[key, fallback] :
         {std::pair{"translate", Vec3{}}, std::pair{"rotate", Vec3{}},
          std::pair{"scale", Vec3{1, 1, 1}}, std::pair{"shear", Vec3{}}}) {
        auto value = Json(Json::Array{Json(fallback.x), Json(fallback.y), Json(fallback.z)});
        auto &stored = fields.try_emplace(key, std::move(value)).first->second;
        const auto vector = stored.vector();
        for (int axis = 0; axis < 3; ++axis)
            if (!std::isfinite(vector[axis]) || std::abs(vector[axis]) > 1e9)
                throw std::runtime_error(std::string("invalid object transform '") + key + "'");
    }
    if (object.has("children"))
        for (auto &child : std::get<Json::Array>(fields.at("children").value))
            normalize(child);
}
} // namespace rt
