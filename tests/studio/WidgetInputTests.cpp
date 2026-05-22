#include "../TestTiming.hpp"
#include "WidgetHarness.hpp"
#include "rt/studio/StudioDocument.hpp"
#include "rt/studio/StudioPresets.hpp"
#include "rt/studio/StudioRenderer.hpp"
#include <fstream>
#include <iostream>
#include <thread>
#include <tuple>

namespace {
using namespace rt;

void check(bool condition, const std::string &message) {
    if (!condition)
        throw std::runtime_error(message);
}

std::vector<Vec3> render(const Json &scene) {
    StudioRenderer renderer;
    renderer.restart(scene);
    const auto deadline = std::chrono::steady_clock::now() + test::budget(10);
    while (std::chrono::steady_clock::now() < deadline) {
        auto state = renderer.progress();
        if (!state.running) {
            check(state.error.empty() && state.control &&
                      state.control->rows() == state.settings.height,
                  "widget scene failed to render: " + state.error);
            return state.control->snapshot();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    throw std::runtime_error("widget render timed out");
}

double difference(const std::vector<Vec3> &a, const std::vector<Vec3> &b) {
    check(a.size() == b.size(), "image size changed");
    double sum = 0;
    for (std::size_t i = 0; i < a.size(); ++i)
        sum += (a[i] - b[i]).length2();
    return sum / static_cast<double>(a.size());
}
} // namespace

int main() {
    try {
        auto scene = Json::parse(R"({
          "camera":{"position":[4,3,7],"target":[0,0.7,0]},
          "render":{"width":64,"height":40,"samples":4,"threads":2,"depth":3},
          "materials":{"red":{"color":[0.8,0.05,0.02]}},
          "objects":[{"type":"box","size":[1.4,1.8,1],"material":"red"}],
          "lights":[{"type":"ambient","intensity":0.2},
                    {"type":"directional","direction":[-1,-2,-1]}]})");
        auto baseline = render(scene);
        auto &object =
            std::get<Json::Array>(std::get<Json::Object>(scene.value).at("objects").value)[0];
        auto preset = StudioPresets::object("box", "red");
        auto &fields = std::get<Json::Object>(object.value);
        for (const char *key : {"translate", "rotate", "scale", "shear"})
            fields[key] = preset.at(key);
        fields["translate"] = Json(Json::Array{Json(0.), Json(0.), Json(0.)});
        check(difference(baseline, render(scene)) == 0, "explicit identities changed rendering");
        for (const auto &[key, text, axis, expected] :
             {std::tuple{"translate", "1.75", 0, 1.75}, std::tuple{"rotate", "37.5", 1, 37.5},
              std::tuple{"scale", "1.4", 1, 1.4}, std::tuple{"shear", "0.2", 2, .2}}) {
            const auto before = render(scene);
            test::WidgetHarness input(object, key);
            input.type(text, axis);
            check(std::abs(object.at(key).vector()[axis] - expected) < 1e-10,
                  std::string("ordinary click + typing failed for ") + key);
            const auto roundtrip = Json::parse(scene.dump());
            check(roundtrip.at("objects").array()[0].at(key).vector()[axis] ==
                      object.at(key).vector()[axis],
                  "edited transform missing from serialized JSON");
            check(difference(before, render(scene)) > 1e-6,
                  std::string(key) + " input did not change actual rendered pixels");
        }
        {
            test::WidgetHarness radius(object, "radius", test::WidgetKind::Number);
            radius.type("0.625");
            check(object.number("radius", 0) == .625, "scalar input did not persist");
            fields.erase("radius");
        }
        const auto file =
            std::filesystem::temp_directory_path() /
            ("raytracer-widget-" +
             std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".json");
        StudioDocument::write(scene, file);
        StudioDocument loaded(file);
        loaded.save(file);
        StudioDocument reopened(file);
        std::filesystem::remove(file);
        for (const char *key : {"translate", "rotate", "scale", "shear"})
            check(reopened.data().at("objects").array()[0].at(key).dump() == object.at(key).dump(),
                  "transform did not survive save/reopen");
        for (const auto &type : StudioPresets::objects()) {
            auto item = StudioPresets::object(type, "red");
            for (const char *key : {"translate", "rotate", "scale", "shear"})
                check(item.has(key), "new primitive lacks explicit transforms");
            if (type == "group")
                check(item.at("children").array()[0].has("rotate"),
                      "group child lacks explicit transforms");
        }
        std::cout << "Ordinary-click numeric editing, serialized transforms, save/reopen and "
                     "live rendered changes passed\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
