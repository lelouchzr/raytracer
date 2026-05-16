#include "rt/engine.hpp"
#include "rt/plugins/PluginLoader.hpp"
#include <dlfcn.h>
#include <iostream>

namespace {
void check(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(message);
}

void unloaded(const char *path) {
#if defined(__linux__) && defined(RTLD_NOLOAD)
    void *handle = dlopen(path, RTLD_NOW | RTLD_NOLOAD);
    if (handle)
        dlclose(handle);
    check(!handle, "plugin image remains loaded after the last lease is released");
#else
    (void)path;
#endif
}
} // namespace

int main(int argc, char **argv) {
    try {
        check(argc == 3, "expected example plugin and rejected-ABI plugin");
        for (int cycle = 0; cycle < 64; ++cycle) {
            rt::Factory::ShapeCreator creator;
            rt::Primitive primitive;
            std::shared_ptr<const rt::IMaterial> material;
            std::shared_ptr<const rt::ILight> light;
            std::weak_ptr<void> lease;
            {
                rt::Factory factory;
                rt::PluginLoader::load(factory, argv[1]);
                rt::PluginLoader::load(factory, argv[1]);
                check(factory.modules.size() == 1, "duplicate load retained another module");
                lease = factory.modules.front();
                creator = factory.shapes.at("plugin_sphere");
                material = factory.materials.at("plugin_material")(
                    rt::Json::parse(R"({"type":"plugin_material"})"), {});
                primitive = creator(rt::Json::parse(R"({"type":"plugin_sphere"})"), material);
                light = factory.lights.at("plugin_light")(
                    rt::Json::parse(R"({"type":"plugin_light"})"));
                bool rejected = false;
                try {
                    rt::PluginLoader::load(factory, argv[2]);
                } catch (const std::runtime_error &) {
                    rejected = true;
                }
                check(rejected && factory.modules.size() == 1,
                      "ABI rejection retained a module or altered existing ownership");
                unloaded(argv[2]);
            }
            check(!lease.expired(), "factory destruction invalidated live plugin objects");
            rt::Hit hit;
            check(primitive->intersect({{0, 0, 3}, {0, 0, -1}}, rt::epsilon, rt::infinity, hit),
                  "leased primitive stopped working after factory destruction");
            check(material->sample({}).color.y > 0 && light->sample({}).radiance.x > 0,
                  "leased material or light stopped working");
            primitive.reset();
            material.reset();
            light.reset();
            check(!lease.expired(), "copied creator lost its plugin lease");
            creator = {};
            check(lease.expired(), "released plugin objects retained the module owner");
            unloaded(argv[1]);
        }
        std::cout << "64 plugin cycles: duplicate loads, ABI rejection, escaped objects, copied "
                     "creators and actual final unloading passed\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
