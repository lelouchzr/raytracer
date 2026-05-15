#include "rt/studio/StudioDocument.hpp"
#include "rt/studio/StudioPresets.hpp"
#include "rt/studio/StudioRenderer.hpp"
#include "rt/studio/StudioWidgets.hpp"
#include <chrono>
#include <iostream>
#include <thread>

namespace {
using namespace rt;
using W = StudioWidgets;

void check(bool condition, const std::string &message) {
    if (!condition)
        throw std::runtime_error(message);
}

Json &member(Json &value, const std::string &key) {
    return std::get<Json::Object>(value.value).at(key);
}

StudioProgress render(StudioRenderer &renderer, const Json &scene) {
    renderer.restart(scene);
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(15);
    while (std::chrono::steady_clock::now() < deadline) {
        auto state = renderer.progress();
        if (!state.running) {
            check(state.error.empty(), state.error);
            check(state.control && state.control->rows() == state.settings.height,
                  "missing completed studio image");
            for (auto pixel : state.control->snapshot())
                for (int channel = 0; channel < 3; ++channel)
                    check(std::isfinite(pixel[channel]) && pixel[channel] >= 0,
                          "invalid displayed radiance");
            return state;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    throw std::runtime_error("studio control render timed out");
}

double difference(const std::vector<Vec3> &a, const std::vector<Vec3> &b) {
    check(a.size() == b.size(), "image dimensions changed unexpectedly");
    double error = 0;
    for (std::size_t i = 0; i < a.size(); ++i)
        error += (a[i] - b[i]).length2();
    return error / static_cast<double>(a.size());
}
} // namespace

int main(int argc, char **argv) {
    try {
        check(argc == 2, "expected project directory");
        StudioDocument document(std::filesystem::path(argv[1]) / "bonus/scenes/pbr_studio.json");
        Json base = document.data();
        auto &settings = std::get<Json::Object>(member(base, "render").value);
        settings["width"] = Json(48.);
        settings["height"] = Json(32.);
        settings["samples"] = Json(4.);
        settings["threads"] = Json(2.);
        settings["depth"] = Json(6.);
        StudioRenderer renderer;
        int cases = 0;
        for (const auto &type : StudioPresets::objects()) {
            auto scene = base;
            auto object = StudioPresets::object(type, "ceramic");
            auto &fields = std::get<Json::Object>(object.value);
            fields["rotate"] = W::vec({15, 30, 10});
            fields["scale"] = W::vec({1.1, .8, 1.2});
            fields["shear"] = W::vec({.15, 0, 0});
            member(scene, "objects") = Json(Json::Array{object});
            try {
                render(renderer, scene);
            } catch (const std::exception &error) {
                throw std::runtime_error("studio primitive " + type + ": " + error.what());
            }
            ++cases;
        }
        for (const auto &type : StudioPresets::lights()) {
            auto scene = base;
            auto light = StudioPresets::light(type);
            std::get<Json::Object>(light.value)["color"] = W::vec({.7, .8, 1});
            member(scene, "lights") = Json(Json::Array{light});
            render(renderer, scene);
            ++cases;
        }
        for (const auto &integrator : {"pbr", "path", "whitted", "photon"}) {
            auto scene = base;
            auto &rendering = std::get<Json::Object>(member(scene, "render").value);
            rendering["integrator"] = Json(std::string(integrator));
            rendering["ao"] = Json(2.);
            rendering["adaptive"] = Json(.01);
            rendering["minSamples"] = Json(2.);
            rendering["photons"] = Json(200.);
            rendering["photonRadius"] = Json(.4);
            for (const auto &tone : {"neutral", "reinhard", "none"})
                for (const auto &encoding : {"srgb", "gamma2"}) {
                    rendering["toneMap"] = Json(std::string(tone));
                    rendering["colorSpace"] = Json(std::string(encoding));
                    render(renderer, scene);
                    ++cases;
                }
        }
        for (const auto &type : {"pbr", "phong", "flat"})
            for (const auto &pattern : {"solid", "checker", "marble", "perlin", "image"}) {
                if (std::string(type) == "flat" && std::string(pattern) != "solid")
                    continue;
                auto scene = base;
                auto material = Json::parse(R"({"type":"pbr","color":[0.3,0.6,0.8]})");
                auto &fields = std::get<Json::Object>(material.value);
                fields["type"] = Json(std::string(type));
                if (std::string(type) != "flat") {
                    fields["pattern"] = Json(std::string(pattern));
                    fields["second"] = W::vec({.05, .1, .15});
                    fields["scale"] = Json(3.);
                    fields["normalFile"] =
                        Json((std::filesystem::path(argv[1]) / "bonus/assets/normal.ppm").string());
                    fields["normalStrength"] = Json(.5);
                    fields["emission"] = W::vec({.01, .02, .03});
                    if (std::string(pattern) == "image")
                        fields["file"] = Json(
                            (std::filesystem::path(argv[1]) / "bonus/assets/mosaic.ppm").string());
                }
                member(member(scene, "materials"), "gold") = std::move(material);
                render(renderer, scene);
                ++cases;
            }
        auto baseline = render(renderer, base).control->snapshot();
        for (const auto &edit : {"geometry", "color", "roughness", "metal", "transmission",
                                 "absorption", "camera", "light", "environment", "exposure"}) {
            auto scene = base;
            auto &gold = member(member(scene, "materials"), "gold");
            auto &glass = member(member(scene, "materials"), "glass");
            auto &root = std::get<Json::Object>(scene.value);
            std::string kind(edit);
            if (kind == "geometry")
                W::member(std::get<Json::Array>(member(scene, "objects").value)[1], "radius",
                          Json(1.)) = Json(.5);
            if (kind == "color")
                W::member(gold, "color", W::vec({})) = W::vec({.01, .1, .8});
            if (kind == "roughness")
                W::member(gold, "roughness", Json(.3)) = Json(.85);
            if (kind == "metal")
                W::member(gold, "metallic", Json(1.)) = Json(0.);
            if (kind == "transmission")
                W::member(glass, "transmission", Json(1.)) = Json(0.);
            if (kind == "absorption")
                W::member(glass, "absorption", W::vec({})) = W::vec({5, .05, 3});
            if (kind == "camera")
                W::member(root.at("camera"), "fov", Json(45.)) = Json(65.);
            if (kind == "light")
                W::member(std::get<Json::Array>(member(scene, "lights").value)[0], "intensity",
                          Json(1.)) = Json(0.);
            if (kind == "environment")
                W::member(root.at("environment"), "rotation", Json(0.)) = Json(135.);
            if (kind == "exposure")
                W::member(root.at("render"), "exposure", Json(1.)) = Json(.3);
            auto changed = render(renderer, scene).control->snapshot();
            check(difference(baseline, changed) > 1e-6,
                  kind + " edit did not change rendered pixels");
            ++cases;
        }
        auto before = document.data().dump();
        for (const auto &bad : {R"({"type":"sphere","material":2})",
                                R"({"type":"sphere","material":"gold","translate":[1,2]})"}) {
            auto invalid = base;
            member(invalid, "objects") = Json(Json::Array{Json::parse(bad)});
            bool rejected = false;
            try {
                document.replace(invalid);
            } catch (const std::exception &) {
                rejected = true;
            }
            check(rejected && document.data().dump() == before,
                  "invalid editor structure corrupted the document");
        }
        std::cout << cases
                  << " studio control configurations rendered; geometry, materials, "
                     "camera, lights, HDR and exposure edits change actual pixels\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
