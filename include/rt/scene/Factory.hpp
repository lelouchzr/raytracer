#pragma once
#include "rt/geometry/IPrimitive.hpp"
#include "rt/json.hpp"
#include "rt/lights/ILight.hpp"
#include <filesystem>
#include <functional>
#include <set>

namespace rt {
class Factory {
  public:
    using ShapeCreator = std::function<Primitive(const Json &, std::shared_ptr<const IMaterial>)>;
    using MaterialCreator = std::function<std::shared_ptr<const IMaterial>(
        const Json &, const std::filesystem::path &)>;
    using LightCreator = std::function<std::shared_ptr<const ILight>(const Json &)>;
    std::vector<std::shared_ptr<void>> modules;
    std::set<std::filesystem::path> loadedPlugins;
    std::map<std::string, ShapeCreator> shapes;
    std::map<std::string, MaterialCreator> materials;
    std::map<std::string, LightCreator> lights;
    Factory();
    Primitive primitive(const Json &, std::shared_ptr<const IMaterial>) const;
};
} // namespace rt
