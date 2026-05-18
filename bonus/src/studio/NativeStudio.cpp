#include "rt/studio/NativeStudio.hpp"
#include "rt/io/ResourcePaths.hpp"
#include "rt/studio/StudioApp.hpp"

namespace rt {
void NativeStudio::run(int argc, char **argv) {
#ifndef RT_NATIVE_STUDIO
    (void)argc;
    (void)argv;
    throw std::runtime_error("native studio requires SDL2 and zlib at build time");
#else
    std::filesystem::path source, screenshot;
    int frames = 0;
    bool haveSource = false;
    for (int i = 0; i < argc; ++i) {
        std::string argument = argv[i];
        if (argument == "--frames" && i + 1 < argc) {
            std::string value = argv[++i];
            std::size_t consumed = 0;
            frames = integer(std::stod(value, &consumed), 1, 100000, "studio frame limit");
            if (consumed != value.size())
                throw std::runtime_error("invalid studio frame limit");
        } else if (argument == "--screenshot" && i + 1 < argc)
            screenshot = argv[++i];
        else if (argument.starts_with("--"))
            throw std::runtime_error("unknown studio option '" + argument + "'");
        else if (!haveSource) {
            source = argument;
            haveSource = true;
        } else
            throw std::runtime_error("studio expects one scene file");
    }
    if (!haveSource)
        source = resourceRoot() / "bonus/scenes/pbr_studio.json";
    StudioApp application(source);
    application.run(frames, screenshot);
#endif
}
} // namespace rt
