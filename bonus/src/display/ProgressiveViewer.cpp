#include "rt/display/ProgressiveViewer.hpp"
#include "rt/math/Color.hpp"
#include "rt/render/AdvancedIntegrator.hpp"
#include "rt/render/RowRenderer.hpp"
#include <chrono>
#include <thread>
#ifdef RT_SFML
#include <SFML/Graphics.hpp>
#endif
namespace rt {
void ProgressiveViewer::run(const std::filesystem::path &file, bool watch, bool preview,
                            int frameLimit) {
#ifndef RT_SFML
    (void)file;
    (void)watch;
    (void)preview;
    (void)frameLimit;
    throw std::runtime_error("GUI requires SFML graphics/window/system at build time");
#else
    auto scene = SceneBuilder(preview, true).build(file);
    if (preview) {
        double ratio = std::min(1., 320. / scene.settings.width);
        scene.settings.width = std::max(1, static_cast<int>(scene.settings.width * ratio));
        scene.settings.height = std::max(1, static_cast<int>(scene.settings.height * ratio));
        scene.settings.samples = 1;
        scene.settings.ao = 0;
        scene.settings.depth = 2;
        scene.settings.integrator = "whitted";
    }
    auto control = std::make_shared<RenderControl>(scene.settings.width, scene.settings.height);
    std::exception_ptr failure;
    std::mutex failureMutex;
    auto launch = [&]() {
        return std::jthread([&]() {
            try {
                if (scene.settings.integrator == "photon")
                    scene.integrator = std::make_shared<AdvancedIntegrator>(
                        scene, std::make_shared<PhotonMap>(scene, control.get()));
                renderImage(scene, *control);
                if (!control->cancelled())
                    writePpm(scene, control->snapshot());
            } catch (...) {
                std::lock_guard lock(failureMutex);
                failure = std::current_exception();
            }
        });
    };
    std::jthread task;
#if SFML_VERSION_MAJOR >= 3
    sf::RenderWindow window(sf::VideoMode({static_cast<unsigned>(scene.settings.width),
                                           static_cast<unsigned>(scene.settings.height)}),
                            "Raytracer progressive");
    sf::Texture texture(sf::Vector2u{static_cast<unsigned>(scene.settings.width),
                                     static_cast<unsigned>(scene.settings.height)});
    sf::Sprite sprite(texture);
#else
    sf::RenderWindow window(sf::VideoMode(static_cast<unsigned>(scene.settings.width),
                                          static_cast<unsigned>(scene.settings.height)),
                            "Raytracer progressive");
    sf::Texture texture;
    if (!texture.create(static_cast<unsigned>(scene.settings.width),
                        static_cast<unsigned>(scene.settings.height)))
        throw std::runtime_error("cannot create GUI texture");
    sf::Sprite sprite(texture);
#endif
    window.setFramerateLimit(30);
    task = launch();
    auto changed = std::filesystem::last_write_time(file);
    int displayed = -1, frames = 0;
    while (window.isOpen()) {
        bool reload = false;
#if SFML_VERSION_MAJOR >= 3
        while (auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>())
                window.close();
            if (auto key = event->getIf<sf::Event::KeyPressed>()) {
                if (key->code == sf::Keyboard::Key::Escape)
                    window.close();
                if (key->code == sf::Keyboard::Key::R)
                    reload = true;
            }
        }
#else
        sf::Event event{};
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Escape)
                    window.close();
                if (event.key.code == sf::Keyboard::R)
                    reload = true;
            }
        }
#endif
        if (watch) {
            std::error_code ec;
            auto stamp = std::filesystem::last_write_time(file, ec);
            if (!ec && stamp != changed) {
                changed = stamp;
                reload = true;
            }
        }
        if (reload) {
            control->cancel();
            if (task.joinable())
                task.join();
            try {
                auto replacement = SceneBuilder(preview, true).build(file);
                replacement.settings.width = scene.settings.width;
                replacement.settings.height = scene.settings.height;
                if (preview) {
                    replacement.settings.samples = 1;
                    replacement.settings.ao = 0;
                    replacement.settings.depth = 2;
                    replacement.settings.integrator = "whitted";
                }
                scene = std::move(replacement);
                control =
                    std::make_shared<RenderControl>(scene.settings.width, scene.settings.height);
                {
                    std::lock_guard lock(failureMutex);
                    failure = nullptr;
                }
                task = launch();
                displayed = -1;
            } catch (const std::exception &e) {
                const auto message = std::string("Reload error: ") + e.what();
                window.setTitle(sf::String::fromUtf8(message.begin(), message.end()));
            }
        }
        if (control->rows() != displayed) {
            displayed = control->rows();
            auto pixels = control->snapshot();
            std::vector<std::uint8_t> rgba(pixels.size() * 4);
            for (std::size_t i = 0; i < pixels.size(); ++i) {
                Vec3 color = pixels[i];
                if (std::filesystem::path(scene.settings.output).extension() == ".pfm")
                    color = toneMap(color * scene.settings.exposure, scene.settings.toneMap);
                for (int c = 0; c < 3; ++c)
                    rgba[i * 4 + static_cast<std::size_t>(c)] = static_cast<std::uint8_t>(
                        255 * displayChannel(color[c], scene.settings.colorSpace) + .5);
                rgba[i * 4 + 3] = 255;
            }
            texture.update(rgba.data());
            window.setTitle("Raytracer " + std::to_string(displayed) + "/" +
                            std::to_string(scene.settings.height) +
                            " rows - R reload / Esc cancel");
        }
        window.clear();
        window.draw(sprite);
        window.display();
        if (frameLimit > 0 && ++frames >= frameLimit)
            window.close();
    }
    control->cancel();
    if (task.joinable())
        task.join();
    std::lock_guard lock(failureMutex);
    if (failure)
        std::rethrow_exception(failure);
#endif
}
} // namespace rt
