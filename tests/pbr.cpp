#include "rt/Extensions.hpp"
#include "rt/io/Image.hpp"
#include "rt/lights/Environment.hpp"
#include "rt/materials/AdvancedMaterial.hpp"
#include "rt/math/Color.hpp"
#include "rt/pbr/Bsdf.hpp"
#include "rt/pbr/PhysicalIntegrator.hpp"
#include "rt/render/MediumStack.hpp"
#include "rt/render/RowRenderer.hpp"
#include <bit>
#include <chrono>
#include <fstream>
#include <iostream>

namespace {
using namespace rt;

void check(bool condition, const char *message) {
    if (!condition)
        throw std::runtime_error(message);
}

void near(double actual, double expected, double tolerance = 1e-6) {
    if (!std::isfinite(actual) || std::abs(actual - expected) > tolerance)
        throw std::runtime_error("PBR mismatch: " + std::to_string(actual) + " expected " +
                                 std::to_string(expected));
}

void bsdfChecks() {
    near(Bsdf::dielectricFresnel(1, 1, 1.5), .04);
    near(Bsdf::dielectricFresnel(0, 1, 1), 0);
    near(Bsdf::dielectricFresnel(.5, 1.5, 1), 1);
    near(PhysicalIntegrator::powerWeight(3, 4), 9. / 25);
    near(PhysicalIntegrator::powerWeight(1e250, 1e250), .5);
    Vec3 wo = unit({.3, 1, .2}), wi = unit({-.2, 1, .5});
    unsigned long long rng = 923;
    for (double roughness : {.12, .4, .85}) {
        Surface material;
        material.physical = true;
        material.color = {.8, .5, .2};
        material.metallic = 1;
        material.roughness = roughness;
        Bsdf bsdf(material, {0, 1, 0});
        near((bsdf.evaluate(wo, wi) - bsdf.evaluate(wi, wo)).length2(), 0);
        Vec3 estimate;
        constexpr int count = 60000;
        for (int i = 0; i < count; ++i) {
            auto sample = bsdf.sample(wo, rng);
            check(std::isfinite(sample.weight.length2()), "non-finite GGX sample");
            estimate += sample.weight;
        }
        estimate = estimate / count;
        check(estimate.x > .3 && estimate.x <= 1.005 && estimate.y <= 1.005 && estimate.z <= 1.005,
              "GGX white furnace violates energy bound");
        std::cout << "GGX roughness=" << roughness << " furnace=" << estimate.x << '\n';
    }
    Surface glass;
    glass.physical = true;
    glass.transmission = 1;
    glass.roughness = .6;
    Bsdf roughGlass(glass, {0, 1, 0});
    constexpr int count = 180000;
    double integratedPdf = 0, valid = 0, sampledEnergy = 0, integratedEnergy = 0;
    for (int i = 0; i < count; ++i) {
        double y = 2 * (i + .5) / count - 1, phi = 0, r = std::sqrt(1 - y * y);
        Vec3 direction{r * std::cos(phi), y, r * std::sin(phi)};
        integratedPdf += roughGlass.pdf({0, 1, 0}, direction) * 4 * pi;
        double etaCorrection = y < 0 ? 2.25 : 1;
        integratedEnergy +=
            roughGlass.evaluate({0, 1, 0}, direction).x * std::abs(y) * 4 * pi * etaCorrection;
        auto sample = roughGlass.sample({0, 1, 0}, rng);
        if (sample.pdf > 0)
            ++valid;
        sampledEnergy += sample.weight.x * (sample.transmitted ? 2.25 : 1);
    }
    near(integratedPdf / count, valid / count, .025);
    near(integratedEnergy / count, sampledEnergy / count, .025);
    check(sampledEnergy / count < 1.01, "rough glass creates energy");
    std::cout << "Rough glass PDF integral=" << integratedPdf / count << " valid=" << valid / count
              << " energy=" << sampledEnergy / count << '\n';
    Bsdf matched(glass, {0, 1, 0}, 1, 1);
    for (int i = 0; i < 100; ++i) {
        auto sample = matched.sample(wo, rng);
        check(sample.delta && sample.transmitted, "matched indices should transmit straight");
        near((sample.direction + wo).length2(), 0);
        near(sample.weight.x, 1);
    }
    glass.roughness = 0;
    Bsdf delta(glass, {0, 1, 0});
    for (int i = 0; i < 1000; ++i) {
        auto sample = delta.sample({0, 1, 0}, rng);
        check(sample.delta, "smooth glass lost delta classification");
        near(sample.weight.x, sample.transmitted ? 1 / 2.25 : 1);
    }
}

void imageAndMedia(const std::filesystem::path &folder) {
    for (double value : {0., .001, .02, .5, 1.})
        near(srgbDecode(srgbEncode(value)), value);
    near(srgbDecode(.5), .21404114048223255);
    MediumStack stack;
    Hit boundary;
    int id = 0;
    boundary.boundary = &id;
    boundary.front = true;
    stack = stack.cross(boundary, 1.5, {.1, .5, 2});
    near(stack.transmittance(2).x, std::exp(-.2));
    near(stack.transmittance(2).z, std::exp(-4));
    boundary.front = false;
    stack = stack.cross(boundary, 1.5);
    near(stack.transmittance(infinity).x, 1);
    auto imagePath = folder / "checker.ppm";
    {
        std::ofstream image(imagePath);
        image << "P3\n16 16\n255\n";
        for (int y = 0; y < 16; ++y)
            for (int x = 0; x < 16; ++x)
                image << ((x + y) % 2 ? "255 255 255\n" : "0 0 0\n");
    }
    Image image(imagePath);
    near(image.sample(.17, .32, 1).x, .5);
    auto hdrPath = folder / "white.hdr";
    {
        std::ofstream hdr(hdrPath, std::ios::binary);
        hdr << "#?RADIANCE\nFORMAT=32-bit_rle_rgbe\n\n-Y 4 +X 4\n";
        for (int i = 0; i < 16; ++i)
            for (int value : {128, 128, 128, 129})
                hdr.put(static_cast<char>(value));
    }
    auto rlePath = folder / "rle.hdr";
    {
        std::ofstream rle(rlePath, std::ios::binary);
        rle << "#?RADIANCE\nFORMAT=32-bit_rle_rgbe\n\n-Y 1 +X 8\n";
        for (int value : {2, 2, 0, 8, 136, 128, 136, 128, 136, 128, 136, 129})
            rle.put(static_cast<char>(value));
    }
    near(Image(rlePath).sample(.4, .5).x, 1);
    {
        std::ofstream broken(folder / "broken.hdr", std::ios::binary);
        broken << "#?RADIANCE\nFORMAT=32-bit_rle_rgbe\n\n-Y 1 +X 8\n";
        for (int value : {2, 2, 0, 8, 0})
            broken.put(static_cast<char>(value));
    }
    bool rejected = false;
    try {
        Image broken(folder / "broken.hdr");
    } catch (const std::exception &) {
        rejected = true;
    }
    check(rejected, "malformed Radiance RLE accepted");
    Image hdr(hdrPath);
    near(hdr.sample(.3, .4).x, 1);
    Environment environment(Json::parse("{\"file\":\"white.hdr\"}"), folder);
    unsigned long long rng = 41;
    Vec3 mean;
    for (int i = 0; i < 30000; ++i) {
        auto sample = environment.sample({}, rng);
        near(sample.pdf, 1 / (4 * pi));
        near(sample.radiance.x, 4 * pi);
        mean += sample.direction;
    }
    check((mean / 30000).length2() < .001, "uniform environment sampling is directional");
    auto material = AdvancedMaterial(
        Json::parse(R"({"type":"pbr","pattern":"checker","color":[1,1,1],"second":[0,0,0]})"),
        folder);
    Hit hit;
    hit.local = {.2, .4, .1};
    hit.footprint = 10;
    near(material.sampleHit(hit).color.x, .5);
}

void transport(const std::filesystem::path &folder) {
    Factory factory;
    registerBonus(factory);
    Scene scene;
    scene.background = {4, 2, .5};
    scene.settings.integrator = "pbr";
    scene.settings.width = 1;
    scene.settings.height = 1;
    scene.settings.samples = 1;
    scene.settings.output = (folder / "linear.pfm").string();
    configureBonus(scene);
    render(scene);
    std::ifstream input(scene.settings.output, std::ios::binary);
    std::string line;
    std::getline(input, line);
    check(line == "PF", "missing PFM signature");
    std::getline(input, line);
    std::getline(input, line);
    std::uint32_t bits = 0;
    for (unsigned shift = 0; shift < 32; shift += 8)
        bits |= std::uint32_t(static_cast<unsigned char>(input.get())) << shift;
    near(std::bit_cast<float>(bits), 4);
    auto diffuse = std::make_shared<AdvancedMaterial>(
        Json::parse(R"({"type":"pbr","color":[0.6,0.6,0.6],"roughness":0.5})"), folder);
    auto emission = std::make_shared<AdvancedMaterial>(
        Json::parse(R"({"type":"pbr","color":[0,0,0],"emission":[3,3,3]})"), folder);
    scene.objects.push_back(factory.primitive(Json::parse(R"({"type":"plane"})"), diffuse));
    scene.background = {};
    scene.settings.depth = 5;
    scene.lights.push_back(factory.lights.at("area")(Json::parse(
        R"({"type":"area","position":[0,2,0],"u":[2,0,0],"v":[0,0,2],"intensity":3})")));
    configureBonus(scene);
    Scene reference = scene;
    reference.lights.clear();
    reference.objects.push_back(factory.primitive(
        Json::parse(R"({"type":"triangle","a":[-1,2,-1],"b":[1,2,-1],"c":[1,2,1]})"), emission));
    reference.objects.push_back(factory.primitive(
        Json::parse(R"({"type":"triangle","a":[-1,2,-1],"b":[1,2,1],"c":[-1,2,1]})"), emission));
    configureBonus(reference);
    unsigned long long rng1 = 7, rng2 = 7;
    double mis = 0, bsdf = 0;
    constexpr int count = 100000;
    for (int i = 0; i < count; ++i) {
        mis += scene.integrator->trace(scene, {{0, 1, 0}, {0, -1, 0}}, 0, rng1).x;
        bsdf += reference.integrator->trace(reference, {{0, 1, 0}, {0, -1, 0}}, 0, rng2).x;
    }
    near(mis / count, bsdf / count, .012);
    std::cout << "MIS=" << mis / count << " BSDF-only=" << bsdf / count << '\n';
}
} // namespace

int main() {
    auto folder =
        std::filesystem::temp_directory_path() /
        ("rt-pbr-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    try {
        std::filesystem::create_directory(folder);
        bsdfChecks();
        imageAndMedia(folder);
        transport(folder);
        std::filesystem::remove_all(folder);
        std::cout << "PBR mathematical checks passed\n";
        return 0;
    } catch (const std::exception &error) {
        std::filesystem::remove_all(folder);
        std::cerr << error.what() << '\n';
        return 1;
    }
}
