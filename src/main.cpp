#include "rt/engine.hpp"
#include <chrono>
#include <iostream>
#ifdef RT_BONUS
#include "rt/display/ProgressiveViewer.hpp"
#include "rt/network/Cluster.hpp"
#endif
namespace {
void usage() {
    std::cout << "USAGE: ./raytracer <SCENE_FILE>\n  SCENE_FILE: JSON, historical or libconfig "
                 "scene configuration\n  Output: P6 PPM (render.output, default output.ppm)\n";
#ifdef RT_BONUS
    std::cout << "  --gui [--watch] [--preview] SCENE   progressive SFML viewer (R reload, Esc "
                 "cancel)\n  --preview SCENE                   fast 320px headless preview\n  "
                 "--cluster HOST:PORT[,HOST:PORT] SCENE  distribute row tiles\n  --worker PORT "
                 "ROOT [BIND_IPV4]      serve RTR1 jobs rooted at ROOT\n";
#endif
}
} // namespace

int main(int argc, char **argv) {
    try {
        if (argc == 2 && (std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h")) {
            usage();
            return 0;
        }
#ifdef RT_BONUS
        if (argc >= 2 && std::string(argv[1]) == "--worker") {
            if (argc < 4 || argc > 5)
                throw std::runtime_error("USAGE: --worker PORT ROOT [BIND_IPV4]");
            std::size_t used = 0;
            int port = std::stoi(argv[2], &used);
            if (used != std::string(argv[2]).size() || port < 1 || port > 65535)
                throw std::runtime_error("invalid worker port");
            rt::Cluster::worker(static_cast<unsigned short>(port), argv[3],
                                argc == 5 ? argv[4] : "127.0.0.1");
            return 0;
        }
        if (argc >= 2 && std::string(argv[1]) == "--cluster") {
            if (argc != 4)
                throw std::runtime_error("USAGE: --cluster HOST:PORT[,HOST:PORT] SCENE");
            rt::Cluster::render(argv[3], argv[2]);
            return 0;
        }
        bool gui = false, watch = false, preview = false;
        int frames = 0;
        std::string file;
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--gui")
                gui = true;
            else if (arg == "--watch") {
                gui = true;
                watch = true;
            } else if (arg == "--preview")
                preview = true;
            else if (arg == "--frames" && i + 1 < argc) {
                std::string argument = argv[++i];
                std::size_t consumed = 0;
                double value = std::stod(argument, &consumed);
                if (consumed != argument.size())
                    throw std::runtime_error("invalid GUI frame limit");
                frames = rt::integer(value, 1, 100000, "GUI frame limit");
            } else if (arg.starts_with("--"))
                throw std::runtime_error("unknown option '" + arg + "'");
            else if (file.empty())
                file = arg;
            else
                throw std::runtime_error("expected one scene argument");
        }
        if (file.empty())
            throw std::runtime_error("USAGE: ./raytracer <SCENE_FILE>");
        if (gui) {
            rt::ProgressiveViewer::run(file, watch, preview, frames);
            return 0;
        }
        if (frames)
            throw std::runtime_error("--frames requires --gui");
#else
        if (argc != 2)
            throw std::runtime_error("USAGE: ./raytracer <SCENE_FILE>");
        std::string file = argv[1];
#endif
        auto start = std::chrono::steady_clock::now();
        auto scene = rt::SceneBuilder(
#ifdef RT_BONUS
                         preview
#endif
                         )
                         .build(file);
        rt::render(scene);
        std::cerr << "Rendered " << scene.settings.width << 'x' << scene.settings.height << " to "
                  << scene.settings.output << " in "
                  << std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count()
                  << "s\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "raytracer: " << e.what() << '\n';
        return 84;
    } catch (...) {
        std::cerr << "raytracer: unexpected error\n";
        return 84;
    }
}
