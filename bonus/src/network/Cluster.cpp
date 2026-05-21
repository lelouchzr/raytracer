#include "rt/network/Cluster.hpp"
#include "rt/concurrency/JoiningThread.hpp"
#include "rt/network/AssetScope.hpp"
#include "rt/network/Socket.hpp"
#include "rt/render/RowRenderer.hpp"
#include <atomic>
#include <bit>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <thread>

namespace rt {
namespace {
std::string quote(const std::string &s) {
    std::string out = "\"";
    for (char raw : s) {
        auto c = static_cast<unsigned char>(raw);
        if (c == '"' || c == '\\') {
            out += '\\';
            out += static_cast<char>(c);
        } else if (c < 32)
            throw std::runtime_error("control characters in network scene path");
        else
            out += static_cast<char>(c);
    }
    return out + '"';
}

void transferColors(const Socket &socket, std::vector<Vec3> &pixels, int width, int begin, int end,
                    bool sending) {
    std::size_t first = static_cast<std::size_t>(begin) * static_cast<std::size_t>(width),
                count = static_cast<std::size_t>(end - begin) * static_cast<std::size_t>(width);
    std::vector<unsigned char> bytes(count * 24);
    if (sending) {
        for (std::size_t i = 0; i < count; ++i)
            for (int c = 0; c < 3; ++c) {
                auto bits = std::bit_cast<std::uint64_t>(pixels[first + i][c]);
                for (int b = 0; b < 8; ++b)
                    bytes[i * 24 + static_cast<std::size_t>(c * 8 + b)] =
                        static_cast<unsigned char>(bits >> (56 - 8 * b));
            }
        socket.send(bytes.data(), bytes.size());
    } else {
        socket.receive(bytes.data(), bytes.size());
        for (std::size_t i = 0; i < count; ++i) {
            double colors[3];
            for (int c = 0; c < 3; ++c) {
                std::uint64_t bits = 0;
                for (int b = 0; b < 8; ++b)
                    bits = (bits << 8) | bytes[i * 24 + static_cast<std::size_t>(c * 8 + b)];
                colors[c] = std::bit_cast<double>(bits);
                if (!std::isfinite(colors[c]) || colors[c] < 0)
                    throw std::runtime_error("invalid worker pixel");
            }
            pixels[first + i] = {colors[0], colors[1], colors[2]};
        }
    }
}
} // namespace

void Cluster::worker(unsigned short port, const std::filesystem::path &root,
                     const std::string &bind) {
    auto canonicalRoot = std::filesystem::canonical(root);
    auto listener = Socket::listen(port, bind);
    std::cerr << "Raytracer worker RTR1 listening " << bind << ':' << port
              << " root=" << canonicalRoot << '\n';
    while (true) {
        auto peer = listener.accept();
        try {
            auto request = Json::parse(peer.receiveFrame());
            keys(request, {"version", "scene", "begin", "end", "width", "height", "samples"});
            if (request.at("version").number() != 1)
                throw std::runtime_error("unsupported RTR protocol version");
            auto relative = std::filesystem::path(request.at("scene").string());
            if (relative.is_absolute())
                throw std::runtime_error("worker scene path must be relative");
            auto file = std::filesystem::canonical(canonicalRoot / relative);
            auto scoped = file.lexically_relative(canonicalRoot);
            if (scoped.empty() || *scoped.begin() == "..")
                throw std::runtime_error("worker path escapes configured root");
            AssetScope(canonicalRoot).validate(file);
            auto scene = SceneBuilder().build(file);
            int width = integer(request.at("width").number(), 1, 8192, "cluster width"),
                height = integer(request.at("height").number(), 1, 8192, "cluster height");
            if (static_cast<long long>(width) * height > 32000000)
                throw std::runtime_error("cluster image too large");
            scene.settings.width = width;
            scene.settings.height = height;
            scene.settings.samples =
                integer(request.at("samples").number(), 1, 4096, "cluster samples");
            int begin = integer(request.at("begin").number(), 0, height, "begin"),
                end = integer(request.at("end").number(), begin, height, "end");
            RenderControl control(width, height);
            renderImage(scene, control, begin, end);
            auto pixels = control.snapshot();
            peer.sendFrame("RTR1 OK " + std::to_string(width) + " " + std::to_string(begin) + " " +
                           std::to_string(end));
            transferColors(peer, pixels, width, begin, end, true);
        } catch (const std::exception &e) {
            try {
                peer.sendFrame(std::string("RTR1 ERROR ") + e.what());
            } catch (...) {
            }
            std::cerr << "Worker request: " << e.what() << '\n';
        }
    }
}

void Cluster::render(const std::filesystem::path &file, const std::string &endpoints) {
    auto scene = SceneBuilder().build(file);
    auto relative =
        std::filesystem::canonical(file).lexically_relative(std::filesystem::current_path());
    if (relative.empty() || *relative.begin() == "..")
        throw std::runtime_error("cluster scene must be inside current project root");
    std::vector<std::pair<std::string, unsigned short>> servers;
    std::istringstream list(endpoints);
    std::string endpoint;
    while (std::getline(list, endpoint, ',')) {
        auto colon = endpoint.rfind(':');
        if (colon == std::string::npos)
            throw std::runtime_error("cluster endpoints require host:port");
        std::size_t used = 0;
        int port = std::stoi(endpoint.substr(colon + 1), &used);
        if (used != endpoint.size() - colon - 1 || port < 1 || port > 65535)
            throw std::runtime_error("invalid cluster port");
        servers.emplace_back(endpoint.substr(0, colon), static_cast<unsigned short>(port));
    }
    if (servers.empty() || servers.size() > 64)
        throw std::runtime_error("cluster requires 1..64 endpoints");
    std::vector<Vec3> pixels(static_cast<std::size_t>(scene.settings.width) *
                             static_cast<std::size_t>(scene.settings.height));
    std::atomic<int> next{0};
    std::atomic<bool> stop{false};
    std::exception_ptr failure;
    std::mutex mutex;
    auto job = [&](const auto &server) {
        try {
            while (!stop) {
                int begin = next.fetch_add(16);
                if (begin >= scene.settings.height)
                    break;
                int end = std::min(begin + 16, scene.settings.height);
                auto socket = Socket::connect(server.first, server.second);
                socket.sendFrame("{\"version\":1,\"scene\":" + quote(relative.generic_string()) +
                                 ",\"begin\":" + std::to_string(begin) +
                                 ",\"end\":" + std::to_string(end) +
                                 ",\"width\":" + std::to_string(scene.settings.width) +
                                 ",\"height\":" + std::to_string(scene.settings.height) +
                                 ",\"samples\":" + std::to_string(scene.settings.samples) + "}");
                auto response = socket.receiveFrame();
                if (response != "RTR1 OK " + std::to_string(scene.settings.width) + " " +
                                    std::to_string(begin) + " " + std::to_string(end))
                    throw std::runtime_error("worker response: " + response);
                transferColors(socket, pixels, scene.settings.width, begin, end, false);
            }
        } catch (...) {
            std::lock_guard lock(mutex);
            if (!failure)
                failure = std::current_exception();
            stop = true;
        }
    };
    std::vector<JoiningThread> workers;
    for (const auto &server : servers)
        workers.emplace_back(job, server);
    workers.clear();
    if (failure)
        std::rethrow_exception(failure);
    writePpm(scene, pixels);
}
} // namespace rt
