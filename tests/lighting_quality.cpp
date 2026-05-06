#include "rt/Extensions.hpp"
#include "rt/lights/Area.hpp"
#include "rt/plugins/LeaseLight.hpp"
#include "rt/render/RowRenderer.hpp"
#include <iostream>
using namespace rt;

namespace {
void check(bool condition, const char *message) {
    if (!condition)
        throw std::runtime_error(message);
}

void near(double actual, double expected, double tolerance = 1e-7) {
    check(std::isfinite(actual) && std::abs(actual - expected) < tolerance,
          "lighting numerical mismatch");
}

std::shared_ptr<Area> ceiling(bool twoSided = false) {
    return std::make_shared<Area>(
        Json::parse(std::string(R"({"type":"area","position":[0,2,0],"u":[2,0,0],"v":[0,0,2],)"
                                R"("intensity":3,"twoSided":)") +
                    (twoSided ? "true}" : "false}")));
}

void analyticNormals() {
    Factory factory;
    registerBonus(factory);
    auto material = std::make_shared<FlatMaterial>(Vec3{.5, .5, .5});
    auto sphere = factory.primitive(Json::parse(R"({"type":"sphere"})"), material);
    auto cylinder = factory.primitive(
        Json::parse(R"({"type":"limited_cylinder","radius":1,"height":2})"), material);
    auto cone = factory.primitive(Json::parse(R"({"type":"limited_cone","radius":1,"height":2})"),
                                  material);
    for (int i = 0; i < 128; ++i) {
        double angle = (i + .37) * 2 * pi / 128;
        Vec3 n{std::cos(angle), 0, std::sin(angle)};
        Hit hit;
        Vec3 radial = unit(Vec3{n.x, .7 * std::sin(3 * angle), n.z});
        check(sphere->intersect({radial * 3, -radial}, epsilon, infinity, hit), "sphere missed");
        near((hit.normal - radial).length2(), 0);
        near((hit.point - radial).length2(), 0);
        Vec3 point = n + Vec3{0, .7, 0};
        check(cylinder->intersect({point + n * 3, -n}, epsilon, infinity, hit), "cylinder missed");
        near((hit.normal - n).length2(), 0);
        near((hit.point - point).length2(), 0);
        point = n * .65 + Vec3{0, .7, 0};
        Vec3 normal = unit(n + Vec3{0, .5, 0});
        check(cone->intersect({point + normal * 2, -normal}, epsilon, infinity, hit),
              "cone missed");
        near((hit.normal - normal).length2(), 0);
        near((hit.point - point).length2(), 0);
    }
    Camera camera;
    camera.position = {};
    camera.target = {0, 0, -1};
    camera.fov = 90;
    Settings settings;
    settings.width = 800;
    settings.height = 400;
    near((camera.ray(400, 200, settings).direction - Vec3{0, 0, -1}).length2(), 0);
    Vec3 top = camera.ray(400, 0, settings).direction;
    near(top.y / -top.z, 1);
    Vec3 right = camera.ray(800, 200, settings).direction;
    near(right.x / -right.z, 2);
}

void geometryAndSampling() {
    auto light = ceiling();
    near(light->sample({0, 0, 0}).radiance.x, 3);
    near(light->sample({0, -2, 0}).radiance.x, .75);
    near(light->sample({0, 4, 0}).radiance.x, 0);
    near(ceiling(true)->sample({0, 4, 0}).radiance.x, 3);
    Vec3 emission;
    double distance = infinity;
    check(light->intersect({{}, {0, 1, 0}}, epsilon, distance, emission), "front emitter missed");
    near(distance, 2);
    near(emission.x, 3);
    distance = infinity;
    check(light->intersect({{0, 4, 0}, {0, -1, 0}}, epsilon, distance, emission),
          "back emitter did not occlude");
    near(emission.length2(), 0);
    distance = infinity;
    check(!light->intersect({{2, 0, 0}, {0, 1, 0}}, epsilon, distance, emission),
          "area emitter behaved as an infinite plane");
    check(!light->intersect({{}, {1, 0, 0}}, epsilon, distance, emission), "parallel ray hit");
    distance = 1;
    check(!light->intersect({{}, {0, 1, 0}}, epsilon, distance, emission),
          "emitter behind geometry");
    auto skew = Area(Json::parse(R"({"position":[0,2,0],"u":[2,0,0],"v":[1,0,2],"intensity":3})"));
    distance = infinity;
    check(skew.intersect({{1.2, 0, .9}, {0, 1, 0}}, epsilon, distance, emission),
          "parallelogram coordinates failed");
    unsigned long long rng = 17;
    Vec3 mean;
    for (int i = 0; i < 30000; ++i) {
        auto sample = light->sample({}, rng);
        Vec3 point = sample.direction * sample.distance;
        check(std::abs(point.x) <= 1 && std::abs(point.z) <= 1, "sample outside emitter");
        near(point.y, 2);
        mean += point;
        Ray photon;
        Vec3 power;
        check(light->emit(rng, photon, power), "area photon emission failed");
        near(power.x, 12 * pi);
        check(photon.direction.y < 0, "one-sided emitter sent photon backward");
        near(photon.direction.length2(), 1);
    }
    near(mean.x / 30000, 0, .015);
    near(mean.z / 30000, 0, .015);
    for (auto invalid : {R"({"position":[0,2,0],"u":[0,0,0],"v":[0,0,2]})",
                         R"({"position":[0,2,0],"u":[1,0,0],"v":[2,0,0]})",
                         R"({"position":[0,2,0],"u":[1,0,0],"v":[0,0,2],"twoSided":1})"}) {
        bool rejected = false;
        try {
            Area bad(Json::parse(invalid));
        } catch (const std::runtime_error &) {
            rejected = true;
        }
        check(rejected, "invalid emitter accepted");
    }
}

void illuminationAndOcclusion() {
    Factory factory;
    registerBonus(factory);
    Scene scene;
    scene.background = {};
    scene.settings.depth = 4;
    scene.settings.integrator = "path";
    auto matte = std::make_shared<FlatMaterial>(Vec3{.6, .6, .6});
    scene.objects.push_back(factory.primitive(Json::parse(R"({"type":"plane"})"), matte));
    auto light = ceiling();
    scene.lights.push_back(light);
    configureBonus(scene);
    double integral = 0;
    constexpr int grid = 400;
    for (int y = 0; y < grid; ++y)
        for (int x = 0; x < grid; ++x) {
            double a = 2 * (x + .5) / grid - 1, b = 2 * (y + .5) / grid - 1;
            double r2 = 4 + a * a + b * b;
            integral += 3 * .6 / pi * 4 / (r2 * r2) * 4 / (grid * grid);
        }
    unsigned long long rng = 51;
    double mean = 0;
    for (int i = 0; i < 40000; ++i)
        mean += scene.integrator->trace(scene, {{0, 1, 0}, {0, -1, 0}}, 0, rng).x;
    near(mean / 40000, integral, .002);
    // This comparison catches counting both NEE and the diffuse ray's hit on the same light.
    scene.settings.depth = 1;
    mean = 0;
    for (int i = 0; i < 40000; ++i)
        mean += scene.integrator->trace(scene, {{0, 1, 0}, {0, -1, 0}}, 0, rng).x;
    near(mean / 40000, integral, .002);
    near(scene.integrator->trace(scene, {{0, 1, 0}, {0, 1, 0}}, 0, rng).x, 3);
    Scene mirror = scene;
    mirror.settings.depth = 4;
    mirror.objects.clear();
    auto silver =
        factory.materials.at("phong")(Json::parse(R"({"type":"phong","reflection":1})"), {});
    mirror.objects.push_back(factory.primitive(Json::parse(R"({"type":"plane"})"), silver));
    configureBonus(mirror);
    near(mirror.integrator->trace(mirror, {{0, 1, 0}, {0, -1, 0}}, 0, rng).x, 3);

    check(visibility(scene, {{0, 1, 0}, {0, 1, 0}}, 3).length2() == 0,
          "emitter failed to block another light");
    check(visibility(scene, {{0, 1, 0}, {0, 1, 0}}, 3, false, light.get()).x == 1,
          "sampled emitter shadowed itself");

    // Compare two estimators of exactly the same finite source, rather than changing the lighting.
    Scene bsdfOnly = scene;
    bsdfOnly.settings.depth = 4;
    bsdfOnly.lights.clear();
    auto emissive = factory.materials.at("phong")(
        Json::parse(R"({"type":"phong","color":[0,0,0],"emission":[3,3,3]})"), {});
    bsdfOnly.objects.push_back(factory.primitive(
        Json::parse(R"({"type":"triangle","a":[-1,2,-1],"b":[1,2,-1],"c":[1,2,1]})"), emissive));
    bsdfOnly.objects.push_back(factory.primitive(
        Json::parse(R"({"type":"triangle","a":[-1,2,-1],"b":[1,2,1],"c":[-1,2,1]})"), emissive));
    configureBonus(bsdfOnly);
    scene.settings.depth = 4;
    double sumNee = 0, squareNee = 0, sumBsdf = 0, squareBsdf = 0;
    unsigned long long rngNee = 123, rngBsdf = 123;
    constexpr int estimates = 40000;
    for (int i = 0; i < estimates; ++i) {
        double a = scene.integrator->trace(scene, {{0, 1, 0}, {0, -1, 0}}, 0, rngNee).x;
        double b = bsdfOnly.integrator->trace(bsdfOnly, {{0, 1, 0}, {0, -1, 0}}, 0, rngBsdf).x;
        sumNee += a;
        squareNee += a * a;
        sumBsdf += b;
        squareBsdf += b * b;
    }
    near(sumNee / estimates, integral, .003);
    near(sumBsdf / estimates, integral, .012);
    double varianceNee = squareNee / estimates - std::pow(sumNee / estimates, 2);
    double varianceBsdf = squareBsdf / estimates - std::pow(sumBsdf / estimates, 2);
    check(varianceNee < varianceBsdf * .1, "NEE failed to reduce finite-light variance");
    std::cout << "Finite-light reference=" << integral << ", NEE mean=" << sumNee / estimates
              << ", BSDF mean=" << sumBsdf / estimates
              << ", variance ratio=" << varianceBsdf / varianceNee << '\n';
    scene.objects.push_back(factory.primitive(
        Json::parse(R"({"type":"sphere","radius":0.25,"translate":[0,1,0]})"), matte));
    configureBonus(scene);
    int blocked = 0;
    for (int i = 0; i < 2000; ++i) {
        auto sample = light->sample({}, rng);
        if (visibility(scene, {{0, epsilon, 0}, sample.direction}, sample.distance, false,
                       light.get())
                .x == 0)
            ++blocked;
    }
    check(blocked > 200 && blocked < 1800, "finite emitter did not create a penumbra");
    Hit hit;
    hit.point = {0, 0, 0};
    hit.geometric = {0, 1, 0};
    near(spawnRay(hit, {1, 1e-12, 0}).origin.y, epsilon);
    near(spawnRay(hit, {1, -1e-12, 0}).origin.y, -epsilon);
}

void displayAndLease(const char *plugin) {
    Scene scene;
    scene.settings.width = 1;
    scene.settings.height = 1;
    scene.settings.samples = 1;
    scene.background = {.25, .5, 2};
    configureBonus(scene);
    RenderControl reinhard(1, 1);
    renderImage(scene, reinhard);
    near(reinhard.snapshot()[0].x, .2);
    scene.settings.toneMap = "none";
    RenderControl linear(1, 1);
    renderImage(scene, linear);
    near(linear.snapshot()[0].x, .25);
    auto library = std::make_shared<SharedLibrary>(plugin);
    LeaseLight leased(library, ceiling());
    unsigned long long a = 77, b = 77;
    auto expected = ceiling()->sample({}, a), actual = leased.sample({}, b);
    near((expected.direction - actual.direction).length2(), 0);
    check(a == b, "plugin light did not forward stochastic sampling");
    Vec3 emission;
    double distance = infinity;
    check(leased.intersect({{}, {0, 1, 0}}, epsilon, distance, emission),
          "plugin light did not forward emitter intersection");
    near(emission.x, 3);
}
} // namespace

int main(int argc, char **argv) {
    try {
        check(argc == 2, "expected example plugin path");
        analyticNormals();
        geometryAndSampling();
        illuminationAndOcclusion();
        displayAndLease(argv[1]);
        std::cout << "Area geometry, radiometric quadrature, NEE energy, penumbra, ray offsets, "
                     "tone mapping and plugin forwarding passed\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
