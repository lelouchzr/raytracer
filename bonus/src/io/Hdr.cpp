#include "rt/io/Hdr.hpp"
#include "rt/engine.hpp"
#include <bit>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <sstream>

namespace rt {
bool decodeRadiance(std::span<const unsigned char> data, int &width, int &height,
                    std::vector<Vec3> &pixels) {
    if (data.size() < 10 ||
        std::string_view(reinterpret_cast<const char *>(data.data()), 2) != "#?")
        return false;
    std::size_t cursor = 0;
    auto line = [&]() {
        std::string text;
        while (cursor < data.size() && data[cursor] != '\n') {
            char value = static_cast<char>(data[cursor++]);
            if (value != '\r')
                text += value;
            if (text.size() > 4096)
                throw std::runtime_error("HDR header line too long");
        }
        if (cursor >= data.size())
            throw std::runtime_error("truncated HDR header");
        ++cursor;
        return text;
    };
    auto magic = line();
    if (magic != "#?RADIANCE" && magic != "#?RGBE")
        throw std::runtime_error("invalid Radiance signature");
    bool format = false;
    for (auto row = line(); !row.empty(); row = line()) {
        if (row == "FORMAT=32-bit_rle_rgbe")
            format = true;
    }
    if (!format)
        throw std::runtime_error("HDR requires FORMAT=32-bit_rle_rgbe");
    std::istringstream resolution(line());
    std::string yAxis, xAxis, extra;
    if (!(resolution >> yAxis >> height >> xAxis >> width) || resolution >> extra ||
        yAxis != "-Y" || xAxis != "+X")
        throw std::runtime_error("HDR requires -Y height +X width orientation");
    integer(width, 1, 8192, "HDR width");
    integer(height, 1, 8192, "HDR height");
    if (static_cast<long long>(width) * height > 32000000)
        throw std::runtime_error("HDR image too large");
    pixels.resize(static_cast<std::size_t>(width) * static_cast<std::size_t>(height));
    auto byte = [&]() {
        if (cursor == data.size())
            throw std::runtime_error("truncated HDR raster");
        return data[cursor++];
    };
    std::vector<unsigned char> row(static_cast<std::size_t>(width) * 4);
    for (int y = 0; y < height; ++y) {
        unsigned char a = byte(), b = byte(), c = byte(), d = byte();
        if (width >= 8 && width <= 32767 && a == 2 && b == 2 && !(c & 128)) {
            if ((unsigned(c) * 256 + d) != static_cast<unsigned>(width))
                throw std::runtime_error("HDR scanline width mismatch");
            for (int channel = 0; channel < 4; ++channel) {
                int x = 0;
                while (x < width) {
                    unsigned code = byte();
                    if (code == 0)
                        throw std::runtime_error("zero HDR RLE packet");
                    int count = static_cast<int>(code > 128 ? code - 128 : code);
                    if (count > width - x)
                        throw std::runtime_error("HDR RLE exceeds scanline");
                    unsigned char value = code > 128 ? byte() : 0;
                    for (int i = 0; i < count; ++i)
                        row[static_cast<std::size_t>((x++) * 4 + channel)] =
                            code > 128 ? value : byte();
                }
            }
        } else {
            row[0] = a;
            row[1] = b;
            row[2] = c;
            row[3] = d;
            for (std::size_t i = 4; i < row.size(); ++i)
                row[i] = byte();
        }
        for (int x = 0; x < width; ++x) {
            auto i = static_cast<std::size_t>(x) * 4;
            double scale = row[i + 3] ? std::ldexp(1., static_cast<int>(row[i + 3]) - 136) : 0;
            pixels[static_cast<std::size_t>(y) * static_cast<std::size_t>(width) +
                   static_cast<std::size_t>(x)] =
                Vec3{double(row[i]), double(row[i + 1]), double(row[i + 2])} * scale;
        }
    }
    if (cursor != data.size())
        throw std::runtime_error("trailing HDR raster data");
    return true;
}

void writePfm(const Scene &scene, const std::vector<Vec3> &pixels) {
    auto temporary = scene.settings.output + ".tmp." +
                     std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    try {
        std::ofstream stream(temporary, std::ios::binary);
        if (!stream)
            throw std::runtime_error("cannot create HDR output");
        stream << "PF\n" << scene.settings.width << ' ' << scene.settings.height << "\n-1.0\n";
        for (int y = scene.settings.height - 1; y >= 0; --y)
            for (int x = 0; x < scene.settings.width; ++x) {
                auto color = pixels.at(static_cast<std::size_t>(y) *
                                           static_cast<std::size_t>(scene.settings.width) +
                                       static_cast<std::size_t>(x));
                for (int channel = 0; channel < 3; ++channel) {
                    double value = color[channel];
                    if (!std::isfinite(value) || value > std::numeric_limits<float>::max())
                        throw std::runtime_error("HDR sample exceeds float range");
                    auto bits = std::bit_cast<std::uint32_t>(static_cast<float>(value));
                    for (unsigned shift = 0; shift < 32; shift += 8)
                        stream.put(static_cast<char>((bits >> shift) & 255));
                }
            }
        stream.flush();
        if (!stream)
            throw std::runtime_error("HDR write failed");
        stream.close();
        if (!stream)
            throw std::runtime_error("HDR close failed");
        std::filesystem::rename(temporary, scene.settings.output);
    } catch (...) {
        std::error_code error;
        std::filesystem::remove(temporary, error);
        throw;
    }
}
} // namespace rt
