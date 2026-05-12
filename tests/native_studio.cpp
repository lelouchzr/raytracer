#include "rt/studio/StudioDocument.hpp"
#include "rt/studio/StudioRenderer.hpp"
#include <chrono>
#include <iostream>
#include <thread>

namespace {
using namespace rt;

void check(bool condition, const char *message) {
    if (!condition)
        throw std::runtime_error(message);
}

StudioProgress wait(StudioRenderer &renderer) {
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while (std::chrono::steady_clock::now() < deadline) {
        auto state = renderer.progress();
        if (!state.running)
            return state;
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    throw std::runtime_error("native renderer timed out");
}
} // namespace

int main(int argc, char **argv) {
    auto folder = std::filesystem::temp_directory_path() /
                  ("native-model-" +
                   std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    try {
        check(argc == 2, "expected project directory");
        std::filesystem::create_directory(folder);
        auto json = Json::parse(
            R"({"text":"quote\" slash\\ newline\n tab\t sc\u00e8ne","array":[true,null,1.23456789012345]})");
        check(Json::parse(json.dump()).dump() == json.dump(), "JSON writer round trip failed");
        check(Json::parse(StudioDocument::formatted(json)).dump() == json.dump(),
              "formatted JSON changed values");
        StudioDocument document(std::filesystem::path(argv[1]) / "bonus/scenes/pbr_studio.json");
        check(std::filesystem::path(document.data().at("environment").at("file").string())
                  .is_absolute(),
              "studio lost resource base directory");
        document.save(folder / "scene.json");
        auto loaded = SceneBuilder(false, true).build(folder / "scene.json");
        check(loaded.settings.integrator == "pbr", "saved scene did not load with engine");
        auto small = document.data();
        auto &settings =
            std::get<Json::Object>(std::get<Json::Object>(small.value).at("render").value);
        settings["width"] = Json(32.);
        settings["height"] = Json(24.);
        settings["samples"] = Json(2.);
        settings["threads"] = Json(1.);
        settings["depth"] = Json(4.);
        StudioRenderer renderer;
        renderer.restart(small);
        auto state = wait(renderer);
        check(state.error.empty() && state.control && state.control->rows() == 24,
              "native progressive renderer failed");
        check(state.control->samples() == 32 * 24 * 2, "native final pass has incorrect budget");
        for (auto value : state.control->completedRows())
            check(value == 1, "missing progressive row publication");
        auto invalid = small;
        std::get<Json::Object>(
            std::get<Json::Array>(std::get<Json::Object>(invalid.value).at("objects").value)[1]
                .value)["radius"] = Json(-1.);
        auto previous = state.control->snapshot();
        renderer.restart(invalid);
        state = wait(renderer);
        check(!state.error.empty(), "invalid edit did not report error");
        check(state.control->snapshot().size() == previous.size(),
              "invalid edit discarded previous image");
        auto hdr = folder / "linear.pfm";
        renderer.restart(small, hdr);
        state = wait(renderer);
        check(state.error.empty() && std::filesystem::file_size(hdr) > 32 * 24 * 12,
              "native HDR export failed");
        settings["width"] = Json(1024.);
        settings["height"] = Json(768.);
        settings["samples"] = Json(4096.);
        auto start = std::chrono::steady_clock::now();
        renderer.restart(small);
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
        renderer.cancel();
        check(std::chrono::steady_clock::now() - start < std::chrono::seconds(3),
              "native render cancellation stalled");
        bool rejected = false;
        try {
            document.replace(Json::parse(R"({"camera":{},"materials":{},"objects":[{}]})"));
        } catch (const std::exception &) {
            rejected = true;
        }
        check(rejected, "invalid scene structure accepted by native editor");
        std::filesystem::remove_all(folder);
        std::cout << "Native document round trips, resource paths, progressive rows, error "
                     "recovery, HDR export and cancellation passed\n";
        return 0;
    } catch (const std::exception &error) {
        std::filesystem::remove_all(folder);
        std::cerr << error.what() << '\n';
        return 1;
    }
}
