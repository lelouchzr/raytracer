#include "rt/plugins/PluginLoader.hpp"
#include "rt/plugins/LeaseLight.hpp"
#include "rt/plugins/LeaseMaterial.hpp"
#include "rt/plugins/LeasePrimitive.hpp"

namespace rt {
void PluginLoader::load(Factory &factory, const std::filesystem::path &path) {
    auto canonical = std::filesystem::canonical(path);
    if (factory.loadedPlugins.count(canonical))
        return;
    auto library = std::make_shared<SharedLibrary>(canonical);
    auto abi = reinterpret_cast<int (*)()>(library->symbol("raytracer_plugin_abi"));
    if (abi() != 1)
        throw std::runtime_error("incompatible raytracer plugin ABI (expected 1)");
    auto oldShapes = factory.shapes;
    auto oldMaterials = factory.materials;
    auto oldLights = factory.lights;
    auto registration =
        reinterpret_cast<void (*)(Factory *)>(library->symbol("raytracer_register"));
    factory.modules.push_back(library);
    try {
        registration(&factory);
        auto untouched = [](const auto &before, const auto &after) {
            for (const auto &[name, creator] : before) {
                auto it = after.find(name);
                if (it == after.end() || it->second.target_type() != creator.target_type())
                    throw std::runtime_error(
                        "plugin may not replace/remove existing registration '" + name + "'");
            }
        };
        untouched(oldShapes, factory.shapes);
        untouched(oldMaterials, factory.materials);
        untouched(oldLights, factory.lights);
    } catch (...) {
        factory.shapes = oldShapes;
        factory.materials = oldMaterials;
        factory.lights = oldLights;
        throw;
    }

    factory.loadedPlugins.insert(canonical);
    for (auto &[name, creator] : factory.shapes)
        if (!oldShapes.count(name)) {
            auto create = creator;
            creator = [library, create](const Json &j, std::shared_ptr<const IMaterial> m) {
                return std::make_shared<LeasePrimitive>(library, create(j, std::move(m)));
            };
        }
    for (auto &[name, creator] : factory.materials)
        if (!oldMaterials.count(name)) {
            auto create = creator;
            creator = [library, create](const Json &j, const std::filesystem::path &base) {
                return std::make_shared<LeaseMaterial>(library, create(j, base));
            };
        }
    for (auto &[name, creator] : factory.lights)
        if (!oldLights.count(name)) {
            auto create = creator;
            creator = [library, create](const Json &j) {
                return std::make_shared<LeaseLight>(library, create(j));
            };
        }
}
} // namespace rt
