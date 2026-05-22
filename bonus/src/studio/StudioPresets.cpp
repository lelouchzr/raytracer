#include "rt/studio/StudioPresets.hpp"
#include "rt/io/ResourcePaths.hpp"
#include "rt/studio/StudioTransforms.hpp"
#include "rt/studio/StudioWidgets.hpp"

namespace rt {
using W = StudioWidgets;

const std::vector<std::string> &StudioPresets::objects() {
    static const std::vector<std::string> types{
        "sphere",     "plane",   "limited_cylinder", "limited_cone", "box",   "triangle", "torus",
        "tanglecube", "fractal", "mobius",           "obj",          "group", "cylinder", "cone"};
    return types;
}

const std::vector<std::string> &StudioPresets::lights() {
    static const std::vector<std::string> types{"area", "point", "directional", "ambient"};
    return types;
}

Json StudioPresets::object(const std::string &type, const std::string &material) {
    Json::Object object{
        {"type", Json(type)}, {"material", Json(material)}, {"translate", W::vec({0, 1, 0})}};
    if (type == "sphere" || type == "cylinder" || type == "cone" || type == "limited_cylinder" ||
        type == "limited_cone")
        object["radius"] = Json(1.);
    if (type == "limited_cylinder" || type == "limited_cone")
        object["height"] = Json(2.);
    if (type == "plane")
        object["normal"] = W::vec({0, 1, 0});
    if (type == "box")
        object["size"] = W::vec({1, 1, 1});
    if (type == "triangle") {
        object["a"] = W::vec({-1, 0, 0});
        object["b"] = W::vec({1, 0, 0});
        object["c"] = W::vec({0, 2, 0});
    }
    if (type == "obj")
        object["file"] = Json((resourceRoot() / "bonus/assets/crystal.obj").string());
    if (type == "group") {
        object.erase("material");
        object["children"] = Json(Json::Array{Json(
            Json::Object{{"type", Json(std::string("sphere"))}, {"material", Json(material)}})});
    }
    Json result(std::move(object));
    StudioTransforms::normalize(result);
    return result;
}

Json StudioPresets::light(const std::string &type) {
    Json::Object light{{"type", Json(type)},
                       {"color", W::vec({1, 1, 1})},
                       {"intensity", Json(type == "area" ? 6. : 1.)}};
    if (type == "area" || type == "point")
        light["position"] = W::vec({-2, 5, 3});
    if (type == "area") {
        light["u"] = W::vec({2, 0, 0});
        light["v"] = W::vec({0, 0, 2});
    }
    if (type == "point") {
        light["physical"] = Json(true);
        light["intensity"] = Json(35.);
    }
    if (type == "directional")
        light["direction"] = W::vec({-1, -2, -1});
    return Json(std::move(light));
}

} // namespace rt
