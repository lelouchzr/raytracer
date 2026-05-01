#include "rt/io/detail/SceneConverter.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <set>
#include <stdexcept>

namespace rt::io {
namespace {

void validate(const ConfigValue &object, std::initializer_list<const char *> allowed) {
    if (object.kind != ConfigKind::Object)
        throw std::runtime_error("expected configuration group");
    for (const auto &[name, value] : object.members) {
        (void)value;
        if (std::none_of(allowed.begin(), allowed.end(),
                         [&](const char *key) { return name == key; }))
            throw std::runtime_error("unknown configuration field '" + name + "'");
    }
}

const ConfigValue *find(const ConfigValue &object, const std::string &key) {
    if (object.kind != ConfigKind::Object)
        throw std::runtime_error("expected configuration group for '" + key + "'");
    const ConfigValue *result = nullptr;
    for (const auto &[name, value] : object.members) {
        if (name != key)
            continue;
        if (result)
            throw std::runtime_error("duplicate configuration field '" + key + "'");
        result = &value;
    }
    return result;
}

const ConfigValue &required(const ConfigValue &object, const std::string &key) {
    const auto *value = find(object, key);
    if (!value)
        throw std::runtime_error("missing configuration field '" + key + "'");
    return *value;
}

double number(const ConfigValue &object, const std::string &key, double fallback) {
    const auto *value = find(object, key);
    if (!value)
        return fallback;
    if (value->kind != ConfigKind::Number)
        throw std::runtime_error("expected number for '" + key + "'");
    return value->number;
}

std::string string(const ConfigValue &object, const std::string &key, const std::string &fallback) {
    const auto *value = find(object, key);
    if (!value)
        return fallback;
    if (value->kind != ConfigKind::String)
        throw std::runtime_error("expected string for '" + key + "'");
    return value->text;
}

Vec3 vector(const ConfigValue &value, bool historical = false, bool rgb = false) {
    if (value.kind == ConfigKind::Array) {
        if (value.elements.size() != 3)
            throw std::runtime_error("configuration vector requires three components");
        Vec3 result;
        double *components[] = {&result.x, &result.y, &result.z};
        for (std::size_t i = 0; i < 3; ++i) {
            if (value.elements[i].kind != ConfigKind::Number)
                throw std::runtime_error("configuration vector components must be numbers");
            *components[i] = value.elements[i].number;
        }
        return result;
    }
    if (value.kind != ConfigKind::Object)
        throw std::runtime_error("expected vector group");
    if (rgb)
        validate(value, {"r", "g", "b", "R", "G", "B"});
    else if (historical)
        validate(value, {"x", "y", "z", "X", "Y", "Z", "X0", "Y0", "Z0", "X1", "Y1", "Z1", "X2",
                         "Y2", "Z2"});
    else
        validate(value, {"x", "y", "z", "X", "Y", "Z"});
    Vec3 result;
    bool seen[3] = {false, false, false};
    std::set<std::string> names;
    double *components[] = {&result.x, &result.y, &result.z};
    for (const auto &[name, component] : value.members) {
        if (!names.insert(name).second)
            throw std::runtime_error("duplicate vector component '" + name + "'");
        if (name.empty())
            continue;
        const char axis = static_cast<char>(std::tolower(static_cast<unsigned char>(name[0])));
        const std::size_t index = axis == 'x' || axis == 'r'   ? 0
                                  : axis == 'y' || axis == 'g' ? 1
                                                               : 2;
        if (axis != 'x' && axis != 'y' && axis != 'z' && axis != 'r' && axis != 'g' && axis != 'b')
            continue;
        // Legacy cylinder groups may contain X2/Y2/Z2 after their first point.
        if (seen[index] && (!historical || name.size() == 1))
            throw std::runtime_error("duplicate vector axis '" + name + "'");
        if (seen[index])
            continue;
        if (component.kind != ConfigKind::Number)
            throw std::runtime_error("vector component is not a number");
        *components[index] = component.number;
        seen[index] = true;
    }
    if (!seen[0] || !seen[1] || !seen[2])
        throw std::runtime_error("configuration vector requires x/y/z or r/g/b");
    return result;
}

Vec3 vectorField(const ConfigValue &object, const std::string &key, Vec3 fallback,
                 bool historical = false) {
    const auto *value = find(object, key);
    return value ? vector(*value, historical) : fallback;
}

Json jsonVector(Vec3 value) {
    return Json(Json::Array{Json(value.x), Json(value.y), Json(value.z)});
}

Json text(const std::string &value) {
    return Json(value);
}

Vec3 color(const ConfigValue &value, bool byteColor) {
    Vec3 result = vector(value, false, true);
    if (byteColor && std::max({result.x, result.y, result.z}) > 1)
        result = result / 255.;
    for (int i = 0; i < 3; ++i)
        range(result[i], 0, 1, "configuration color");
    return result;
}

Vec3 colorField(const ConfigValue &object, const std::string &key, Vec3 fallback, bool bytes) {
    const auto *value = find(object, key);
    return value ? color(*value, bytes) : fallback;
}

Vec3 rotated(Vec3 point, Vec3 degrees) {
    for (int axis = 0; axis < 3; ++axis) {
        const double angle = degrees[axis] * pi / 180.;
        const double cosine = std::cos(angle);
        const double sine = std::sin(angle);
        if (axis == 0)
            point = {point.x, cosine * point.y - sine * point.z, sine * point.y + cosine * point.z};
        else if (axis == 1)
            point = {cosine * point.x + sine * point.z, point.y,
                     -sine * point.x + cosine * point.z};
        else
            point = {cosine * point.x - sine * point.y, sine * point.x + cosine * point.y, point.z};
    }
    return point;
}

Json::Object transformation(const ConfigValue &primitive, bool historical) {
    Json::Object result;
    if (const auto *translation = find(primitive, "translate"))
        result["translate"] = jsonVector(vector(*translation, historical));
    const auto *rotation = find(primitive, historical ? "rotate" : "rotation");
    if (!rotation)
        rotation = find(primitive, "rotate");
    if (rotation) {
        const auto *angle =
            rotation->kind == ConfigKind::Object ? find(*rotation, "angle") : nullptr;
        if (angle)
            validate(*rotation, {"angle"});
        result["rotate"] = jsonVector(angle ? Vec3{0, number(*rotation, "angle", 0), 0}
                                            : vector(*rotation, historical));
    }
    if (const auto *scale = find(primitive, "scale")) {
        Vec3 value = vector(*scale, historical);
        // The original parser accepted all-zero scales but never applied them.
        if (historical && value.length2() == 0)
            value = {1, 1, 1};
        result["scale"] = jsonVector(value);
    }
    return result;
}

std::string singular(const std::string &name) {
    if (name == "spheres")
        return "sphere";
    if (name == "planes")
        return "plane";
    if (name == "cylinders")
        return "cylinder";
    if (name == "cones")
        return "cone";
    if (name == "triangles")
        return "triangle";
    throw std::runtime_error("unsupported libconfig primitive group '" + name + "'");
}

} // namespace

SceneConverter::SceneConverter(const ConfigValue &configuration) : configuration_(configuration) {
    const auto &description = required(configuration_, "camera");
    historical_ = find(description, "camera_position") != nullptr;
}

Json SceneConverter::camera() const {
    const auto &description = required(configuration_, "camera");
    if (historical_)
        validate(description,
                 {"camera_position", "camera_direction", "camera_fov", "camera_size", "end"});
    else
        validate(description, {"position", "rotation", "target", "resolution", "fieldOfView"});
    const Vec3 position = vectorField(description, historical_ ? "camera_position" : "position",
                                      historical_ ? Vec3{0, 2, 7} : Vec3{0, -100, 20}, historical_);
    const Vec3 rotation = vectorField(description, "rotation", {});
    const Vec3 target =
        historical_ ? vectorField(description, "camera_direction", {}, true)
                    : vectorField(description, "target", position + rotated({0, 1, 0}, rotation));
    const Vec3 up = historical_ ? Vec3{0, 1, 0} : rotated({0, 0, 1}, rotation);
    return Json(Json::Object{
        {"position", jsonVector(position)},
        {"target", jsonVector(target)},
        {"up", jsonVector(up)},
        {"fov", Json(number(description, historical_ ? "camera_fov" : "fieldOfView", 60))}});
}

Json SceneConverter::renderSettings() const {
    const auto &description = required(configuration_, "camera");
    const auto *resolution = find(description, historical_ ? "camera_size" : "resolution");
    if (resolution)
        validate(*resolution, {"width", "height"});
    const double width = resolution ? number(*resolution, "width", 640) : 640;
    const double height = resolution ? number(*resolution, "height", 360) : 360;
    return Json(Json::Object{{"width", Json(width)},
                             {"height", Json(height)},
                             {"samples", Json(4.)},
                             {"threads", Json(0.)},
                             {"output", text("output.ppm")}});
}

std::string SceneConverter::material(const ConfigValue &primitive) {
    const std::string name = "imported_" + std::to_string(materials_.size());
    Json::Object result;
    if (!historical_) {
        result = {{"type", text("flat")},
                  {"color", jsonVector(colorField(primitive, "color", {.7, .7, .7}, true))}};
    } else {
        const auto &description = required(primitive, "material");
        validate(description, {"color", "alpha", "material_type", "end"});
        range(number(description, "alpha", 0), 0, 1, "legacy material alpha");
        const int type =
            integer(number(description, "material_type", 0), 0, 5, "legacy material type");
        const auto *rawColor = find(description, "color");
        const auto raw = rawColor ? vector(*rawColor, false, true) : Vec3{.7, .7, .7};
        Vec3 albedo = raw;
        if (type == 2) {
            const double maximum = std::max({1., raw.x, raw.y, raw.z});
            albedo = raw / maximum;
        }
        for (int i = 0; i < 3; ++i)
            range(albedo[i], 0, 1, "legacy material color");
        result = {{"type", text("phong")}, {"color", jsonVector(albedo)}, {"specular", Json(0.)}};
        if (type == 1) {
            result["reflection"] = Json(.8);
            result["metallic"] = Json(1.);
            result["specular"] = Json(.7);
        } else if (type == 2) {
            result["emission"] = jsonVector(raw);
        } else if (type == 3 || type == 4) {
            const auto *checker = find(configuration_, "checker");
            result["pattern"] = text(type == 3 ? "checker" : "marble");
            result["scale"] = Json(checker ? number(*checker, "noise_value", 1) : 1);
            if (checker) {
                result["color"] = jsonVector(colorField(*checker, "checker_color1", albedo, false));
                result["second"] =
                    jsonVector(colorField(*checker, "checker_color2", {.1, .1, .1}, false));
            }
        } else if (type == 5) {
            const auto *checker = find(configuration_, "checker");
            result["transmission"] = Json(.98);
            result["ior"] = Json(checker ? number(*checker, "dielectric_value", 1.5) : 1.5);
        }
    }
    materials_[name] = Json(std::move(result));
    return name;
}

void SceneConverter::historicalPrimitive(const std::string &type, const ConfigValue &primitive) {
    if (type == "plane")
        validate(primitive, {"coefficient", "material", "translate", "rotate", "scale", "end"});
    else if (type == "sphere")
        validate(primitive,
                 {"position", "radius", "material", "translate", "rotate", "scale", "end"});
    else
        validate(primitive, {"position", "radius", "height", "material", "translate", "rotate",
                             "scale", "end"});
    const auto materialName = material(primitive);
    auto result = transformation(primitive, true);
    result["material"] = text(materialName);
    if (type == "plane") {
        std::vector<Vec3> coefficients;
        for (const auto &[name, group] : primitive.members)
            if (name == "coefficient")
                coefficients.push_back(vector(group, true));
        if (coefficients.size() != 3)
            throw std::runtime_error("legacy plane needs origin and two edge vectors");
        const Vec3 origin = coefficients[0];
        const Vec3 first = origin + coefficients[1];
        const Vec3 second = origin + coefficients[2];
        result["type"] = text("triangle");
        result["a"] = jsonVector(origin);
        result["b"] = jsonVector(first);
        result["c"] = jsonVector(second);
        objects_.emplace_back(result);
        result["a"] = jsonVector(first);
        result["b"] = jsonVector(first + coefficients[2]);
        objects_.emplace_back(std::move(result));
        return;
    }
    if (type != "sphere" && type != "cylinder" && type != "cone")
        throw std::runtime_error("unsupported historical primitive '" + type + "'");
    const Vec3 translation = vectorField(primitive, "position", {}, true) +
                             vectorField(primitive, "translate", {}, true);
    result["translate"] = jsonVector(translation);
    result["type"] = text(type);
    result["radius"] = Json(number(primitive, "radius", 1));
    if (type == "cylinder") {
        const auto &description = required(primitive, "position");
        const bool hasEnd =
            find(description, "X2") || find(description, "Y2") || find(description, "Z2");
        if (hasEnd) {
            for (const auto *component : {"X2", "Y2", "Z2"})
                if (required(description, component).kind != ConfigKind::Number)
                    throw std::runtime_error("legacy cylinder endpoints must be numbers");
            const Vec3 end{number(description, "X2", 0), number(description, "Y2", 0),
                           number(description, "Z2", 0)};
            const Vec3 axis = end - vector(description, true);
            const double height = std::sqrt(axis.length2());
            range(height, 1e-6, 1e6, "legacy cylinder height");
            const auto direction = axis / height;
            result["type"] = text("limited_cylinder");
            result["height"] = Json(height);
            result["rotate"] = jsonVector({std::asin(direction.z) * 180 / pi, 0,
                                           std::atan2(-direction.x, direction.y) * 180 / pi});
        }
    }
    if (type == "cone" && find(primitive, "height")) {
        result["type"] = text("limited_cone");
        result["height"] = Json(number(primitive, "height", 2));
    }
    objects_.emplace_back(std::move(result));
}

void SceneConverter::standardPrimitive(const std::string &type, const ConfigValue &primitive) {
    if (type == "plane")
        validate(primitive, {"x", "y", "z", "axis", "position", "normal", "color", "translate",
                             "rotate", "rotation", "scale"});
    else if (type == "triangle")
        validate(primitive, {"x", "y", "z", "a", "b", "c", "color", "translate", "rotate",
                             "rotation", "scale"});
    else if (type == "sphere")
        validate(primitive, {"x", "y", "z", "r", "radius", "color", "translate", "rotate",
                             "rotation", "scale"});
    else
        validate(primitive, {"x", "y", "z", "r", "radius", "height", "color", "translate", "rotate",
                             "rotation", "scale"});
    auto result = transformation(primitive, false);
    result["type"] = text(type);
    result["material"] = text(material(primitive));
    Vec3 position{number(primitive, "x", 0), number(primitive, "y", 0), number(primitive, "z", 0)};
    position = position + vectorField(primitive, "translate", {});
    if (type == "plane") {
        const auto axis = string(primitive, "axis", "Y");
        if (axis != "X" && axis != "Y" && axis != "Z")
            throw std::runtime_error("plane axis must be X, Y or Z");
        Vec3 normal = axis == "X" ? Vec3{1, 0, 0} : axis == "Y" ? Vec3{0, 1, 0} : Vec3{0, 0, 1};
        normal = vectorField(primitive, "normal", normal);
        position = position + unit(normal) * number(primitive, "position", 0);
        result["normal"] = jsonVector(normal);
    } else if (type == "triangle") {
        result["a"] = jsonVector(vector(required(primitive, "a")));
        result["b"] = jsonVector(vector(required(primitive, "b")));
        result["c"] = jsonVector(vector(required(primitive, "c")));
    } else {
        result["radius"] = Json(number(primitive, "r", number(primitive, "radius", 1)));
        if ((type == "cone" || type == "cylinder") && find(primitive, "height")) {
            result["type"] = text(type == "cone" ? "limited_cone" : "limited_cylinder");
            result["height"] = Json(number(primitive, "height", 2));
        }
    }
    result["translate"] = jsonVector(position);
    objects_.emplace_back(std::move(result));
}

Json SceneConverter::lights() const {
    Json::Array result;
    const auto *description = find(configuration_, historical_ ? "light" : "lights");
    if (!description)
        return Json(std::move(result));
    if (historical_) {
        validate(*description, {"ambient_light", "directional_light", "end"});
        for (const auto &[kind, light] : description->members) {
            if (kind == "end")
                continue;
            const bool ambient = kind == "ambient_light";
            if (!ambient && kind != "directional_light")
                throw std::runtime_error("unsupported legacy light '" + kind + "'");
            if (ambient)
                validate(light, {"ambient_light_color", "end"});
            else
                validate(light, {"directional_light_color", "direction", "end"});
            Json::Object entry{
                {"type", text(ambient ? "ambient" : "directional")},
                {"color", jsonVector(color(required(light, kind + "_color"), false))}};
            if (!ambient)
                entry["direction"] = jsonVector(vectorField(light, "direction", {-1, -2, -1}));
            result.emplace_back(std::move(entry));
        }
    } else {
        validate(*description, {"ambient", "diffuse", "point", "directional"});
        result.emplace_back(Json::Object{{"type", text("ambient")},
                                         {"intensity", Json(number(*description, "ambient", .1))}});
        const double diffuse = number(*description, "diffuse", 1);
        for (const auto &kind : {std::string("directional"), std::string("point")}) {
            const auto *entries = find(*description, kind);
            if (!entries)
                continue;
            if (entries->kind != ConfigKind::Array)
                throw std::runtime_error("light list requires '(...)'");
            for (const auto &light : entries->elements) {
                if (kind == "point")
                    validate(light, {"x", "y", "z", "position", "color", "intensity", "falloff"});
                else
                    validate(light, {"x", "y", "z", "direction", "color", "intensity"});
                Json::Object entry{
                    {"type", text(kind)},
                    {"color", jsonVector(colorField(light, "color", {1, 1, 1}, true))},
                    {"intensity", Json(diffuse * number(light, "intensity", 1))}};
                const Vec3 coordinates{number(light, "x", 0), number(light, "y", 0),
                                       number(light, "z", 0)};
                const auto field = kind == "point" ? "position" : "direction";
                entry[field] = jsonVector(vectorField(light, field, coordinates));
                if (kind == "point" && find(light, "falloff"))
                    entry["falloff"] = Json(number(light, "falloff", 0));
                result.emplace_back(std::move(entry));
            }
        }
    }
    return Json(std::move(result));
}

Json SceneConverter::convert() {
    if (historical_)
        validate(configuration_, {"camera", "primitives", "light", "checker"});
    else
        validate(configuration_, {"camera", "primitives", "lights"});
    if (const auto *checker = find(configuration_, "checker"))
        validate(*checker,
                 {"checker_color1", "checker_color2", "dielectric_value", "noise_value", "end"});
    const auto &primitives = required(configuration_, "primitives");
    if (primitives.kind != ConfigKind::Object)
        throw std::runtime_error("primitives must be a configuration group");
    for (const auto &[kind, description] : primitives.members) {
        if (kind == "end")
            continue;
        if (historical_) {
            historicalPrimitive(kind, description);
        } else {
            (void)find(primitives, kind);
            const auto type = singular(kind);
            if (description.kind != ConfigKind::Array)
                throw std::runtime_error("primitive list requires '(...)'");
            for (const auto &primitive : description.elements)
                standardPrimitive(type, primitive);
        }
    }
    Vec3 background;
    const auto illumination = lights();
    if (historical_)
        for (const auto &light : illumination.array())
            if (light.at("type").string() == "ambient")
                background = light.at("color").vector();
    return Json(Json::Object{{"camera", camera()},
                             {"render", renderSettings()},
                             {"background", jsonVector(background)},
                             {"materials", Json(std::move(materials_))},
                             {"objects", Json(std::move(objects_))},
                             {"lights", illumination}});
}

} // namespace rt::io
