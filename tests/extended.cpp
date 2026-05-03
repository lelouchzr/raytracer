#include "rt/engine.hpp"
#include "rt/io/Image.hpp"
#include "rt/materials/AdvancedMaterial.hpp"
#include "rt/materials/Perlin.hpp"
#include "rt/math/Mat4.hpp"
#include "rt/plugins/PluginLoader.hpp"
#include "rt/render/MediumStack.hpp"
#include "rt/render/PhotonMap.hpp"
#include "rt/render/RowRenderer.hpp"
#include <fstream>
#include <iostream>
#include <thread>
using namespace rt;

namespace {
void check(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(message);
}

void near(double a, double b, double tolerance = 1e-7) {
    check(std::abs(a - b) < tolerance, "numerical mismatch");
}
} // namespace

int main(int argc, char **argv) {
    try {
        check(argc == 3, "expected project root");
        std::filesystem::path root = argv[1];
        Factory factory;
        registerBonus(factory);
        auto material = std::make_shared<FlatMaterial>(Vec3{.8, .8, .8});
        Hit hit;
        auto torus = factory.primitive(Json::parse(R"({"type":"torus","major":1.3,"minor":0.35})"),
                                       material);
        check(torus->intersect({{0, 0, 3}, {0, 0, -1}}, epsilon, infinity, hit), "torus missed");
        near(hit.t, 1.35, 1e-4);
        check(torus->intersect({{0, 0, 1.3}, {0, 0, 1}}, epsilon, infinity, hit),
              "inside torus missed");
        near(hit.t, .35, 1e-4);
        for (auto name : {"tanglecube", "fractal"}) {
            Json::Object shape{{"type", Json(std::string(name))}};
            auto object = factory.primitive(Json(shape), material);
            check(object->intersect({{name == std::string("tanglecube") ? 1.4 : 0,
                                      name == std::string("tanglecube") ? 1.4 : 0, 5},
                                     {0, 0, -1}},
                                    epsilon, infinity, hit),
                  "implicit missed");
            check(std::isfinite(hit.normal.length2()), "implicit invalid normal");
        }
        auto mobius = factory.primitive(Json::parse(R"({"type":"mobius"})"), material);
        check(mobius->intersect({{1.5, 3, 0}, {0, -1, 0}}, epsilon, infinity, hit),
              "Mobius missed");
        Mat4 matrix = Mat4::fromObject(Json::parse(
            R"({"translate":[1,2,3],"rotate":[10,20,30],"shear":[0.3,0.2,0.1],"scale":[2,1,0.5]})"));
        Vec3 p{.3, .6, .8};
        near((matrix.inverse().point(matrix.point(p)) - p).length2(), 0);
        auto affine = factory.primitive(
            Json::parse(
                R"({"type":"sphere","shear":[0.5,0,0],"matrix":[1,0,0,0,0,1,0,0,0,0,1,-4,0,0,0,1]})"),
            material);
        check(affine->intersect({{0, 0, 0}, {0, 0, -1}}, epsilon, infinity, hit),
              "affine sphere missed");
        near(hit.t, 3);
        auto mirrored = factory.primitive(
            Json::parse(R"({"type":"sphere","scale":[-2,1,1],"translate":[0,0,-4]})"), material);
        check(mirrored->intersect({{0, 0, 0}, {0, 0, -1}}, epsilon, infinity, hit),
              "negative scale missed");
        near(hit.t, 3);
        check(hit.front, "negative scale changed front face");
        MediumStack media;
        int outer = 0, inner = 0;
        Hit crossing;
        crossing.front = true;
        crossing.boundary = &outer;
        auto indices = media.indices(crossing, 1.5);
        near(indices.first, 1);
        near(indices.second, 1.5);
        media = media.cross(crossing, 1.5);
        crossing.boundary = &inner;
        indices = media.indices(crossing, 1.33);
        near(indices.first, 1.5);
        near(indices.second, 1.33);
        media = media.cross(crossing, 1.33);
        crossing.front = false;
        indices = media.indices(crossing, 1.33);
        near(indices.first, 1.33);
        near(indices.second, 1.5);
        media = media.cross(crossing, 1.33);
        crossing.boundary = &outer;
        indices = media.indices(crossing, 1.5);
        near(indices.first, 1.5);
        near(indices.second, 1);
        Scene nested;
        auto outerMaterial = std::make_shared<AdvancedMaterial>(
            Json::parse(R"({"type":"phong","transmission":1,"ior":1.5})"), root);
        auto innerMaterial = std::make_shared<AdvancedMaterial>(
            Json::parse(R"({"type":"phong","transmission":1,"ior":1.33})"), root);
        nested.objects.push_back(
            factory.primitive(Json::parse(R"({"type":"sphere","radius":2})"), outerMaterial));
        nested.objects.push_back(
            factory.primitive(Json::parse(R"({"type":"sphere","radius":1})"), innerMaterial));
        MediumStack inside = MediumStack::atOrigin(nested, {0, 0, 0});
        near(inside.current(), 1.33);
        Hit exit;
        check(nested.intersect({{0, 0, 0}, {1, 0, 0}}, epsilon, infinity, exit),
              "nested internal exit missed");
        auto internalIndices = inside.indices(exit, 1.33);
        near(internalIndices.first, 1.33);
        near(internalIndices.second, 1.5);
        Perlin perlin;
        near(perlin.noise({0, 0, 0}), 0);
        double n = perlin.noise({.2, .3, .4});
        check(std::abs(n) > 1e-4 && std::abs(n) < 1.1, "Perlin lacks gradients");
        near(perlin.noise({.2 + 256, .3, .4}), n);
        near(perlin.noise({1.000001, .2, .3}), perlin.noise({.999999, .2, .3}), 1e-4);
        Scene gi;
        gi.background = {};
        gi.settings.depth = 4;
        gi.settings.integrator = "path";
        gi.objects.push_back(factory.primitive(Json::parse(R"({"type":"plane"})"), material));
        auto emitter = std::make_shared<AdvancedMaterial>(
            Json::parse(R"({"type":"phong","color":[0,0,0],"emission":[2,1,0.5]})"), root);
        gi.objects.push_back(
            factory.primitive(Json::parse(R"({"type":"plane","translate":[0,4,0]})"), emitter));
        configureBonus(gi);
        unsigned long long rng = 1;
        Vec3 indirect = gi.integrator->trace(gi, {{0, 1, 0}, {0, -1, 0}}, 0, rng);
        check(indirect.x > .5, "path tracer failed indirect emissive illumination");
        gi.settings.integrator = "whitted";
        rng = 1;
        check(gi.integrator->trace(gi, {{0, 1, 0}, {0, -1, 0}}, 0, rng).length2() == 0,
              "whitted unexpectedly includes diffuse GI");
        auto physicalPoint = factory.lights.at("point")(
            Json::parse(R"({"type":"point","position":[0,0,0],"physical":true})"));
        near(physicalPoint->sample({0, 0, 2}).radiance.x /
                 physicalPoint->sample({0, 0, 4}).radiance.x,
             4);
        Scene caustic;
        caustic.settings.photons = 30000;
        caustic.settings.depth = 8;
        caustic.settings.photonRadius = .4;
        caustic.objects.push_back(factory.primitive(Json::parse(R"({"type":"plane"})"), material));
        caustic.lights.push_back(factory.lights.at("point")(
            Json::parse(R"({"type":"point","position":[0,5,0],"intensity":10})")));
        PhotonMap noSpecular(caustic);
        check(noSpecular.size() == 0, "caustic map deposited direct diffuse photons");
        auto glass = std::make_shared<AdvancedMaterial>(
            Json::parse(R"({"type":"phong","transmission":0.94,"reflection":0.03,"ior":1.5})"),
            root);
        caustic.objects.push_back(factory.primitive(
            Json::parse(R"({"type":"sphere","radius":0.7,"translate":[0,2,0]})"), glass));
        PhotonMap photons(caustic);
        Hit receiver;
        check(caustic.intersect({{3, 1, 0}, {0, -1, 0}}, epsilon, infinity, receiver),
              "caustic receiver missed");
        check(photons.countFor(receiver.boundary) > 50, "partial glass trapped caustics on shell");
        check(photons.size() > 50, "no light-specular-diffuse photon deposits");
        check(photons.radiance({0, 0, 0}, {0, 1, 0}, {1, 1, 1}).length2() > 0,
              "caustic gather has no radiance");
        Scene uniform = gi;
        uniform.settings.width = 24;
        uniform.settings.height = 16;
        uniform.settings.samples = 64;
        uniform.settings.minSamples = 4;
        uniform.settings.adaptive = .0001;
        uniform.settings.threads = 2;
        uniform.settings.integrator = "whitted";
        uniform.objects.clear();
        uniform.background = {.2, .2, .2};
        configureBonus(uniform);
        RenderControl adaptive(24, 16);
        renderImage(uniform, adaptive);
        check(adaptive.samples() == 24 * 16 * 4,
              "adaptive AA did not stop constant pixels at minimum");
        RenderControl cancelled(24, 16);
        cancelled.cancel();
        renderImage(uniform, cancelled);
        check(cancelled.rows() == 0, "cancellation ignored");
        Primitive escaped;
        {
            Factory pluginFactory;
            PluginLoader::load(pluginFactory, argv[2]);
            escaped = pluginFactory.primitive(Json::parse(R"({"type":"plugin_sphere"})"), material);
        }
        check(escaped->intersect({{0, 0, 3}, {0, 0, -1}}, epsilon, infinity, hit),
              "plugin lease expired with factory");
        near(hit.t, 2);
        escaped.reset();
        std::weak_ptr<void> materialLease;
        Primitive copiedObject;
        {
            Factory plugins;
            PluginLoader::load(plugins, argv[2]);
            materialLease = plugins.modules.front();
            auto pluginMaterial = plugins.materials.at("plugin_material")(
                Json::parse(R"({"type":"plugin_material"})"), root);
            copiedObject = plugins.primitive(Json::parse(R"({"type":"sphere"})"), pluginMaterial);
        }
        check(!materialLease.expired(), "plugin material did not retain library");
        check(copiedObject->intersect({{0, 0, 3}, {0, 0, -1}}, epsilon, infinity, hit),
              "copied object with plugin material failed");
        near(hit.material->sampleHit(hit).color.y, .7);
        copiedObject.reset();
        check(materialLease.expired(), "plugin material library leaked");
        std::weak_ptr<void> lightLease;
        std::shared_ptr<const ILight> copiedLight;
        {
            Factory plugins;
            PluginLoader::load(plugins, argv[2]);
            lightLease = plugins.modules.front();
            copiedLight =
                plugins.lights.at("plugin_light")(Json::parse(R"({"type":"plugin_light"})"));
        }
        check(!lightLease.expired(), "plugin light did not retain library");
        check(copiedLight->sample({}).radiance.x > 0, "copied plugin light failed");
        copiedLight.reset();
        check(lightLease.expired(), "plugin light library leaked");
        auto scene = SceneBuilder().build(root / "bonus/scenes/scenegraph.json");
        check(scene.intersect({{0, 1, 8}, {0, 0, -1}}, epsilon, infinity, hit),
              "scene graph hierarchy missed");
        for (const auto &file :
             std::filesystem::recursive_directory_iterator(root / "tests/legacy")) {
            if (file.path().extension() != ".txt" || file.path().filename() == "config.txt" ||
                file.path().filename() == "final.txt")
                continue;
            auto historic = SceneBuilder().build(file.path());
            historic.settings.width = 12;
            historic.settings.height = 8;
            historic.settings.samples = 1;
            historic.settings.depth = 3;
            historic.settings.ao = 0;
            historic.settings.threads = 1;
            RenderControl historicPixels(12, 8);
            renderImage(historic, historicPixels);
            for (Vec3 color : historicPixels.snapshot())
                check(std::isfinite(color.length2()), "historical scene rendered non-finite pixel");
        }
        std::cout << "Extended geometry, matrices, media, Perlin, GI, photon caustics, "
                     "adaptive/cancel and plugin lifetime passed\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
