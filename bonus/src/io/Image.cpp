#include "rt/io/Image.hpp"
#include "rt/io/Hdr.hpp"
#include "rt/json.hpp"
#include "rt/math/Color.hpp"
#include <cctype>
#include <fstream>

namespace rt {
Image::Image(const std::filesystem::path &path, bool linearize) {
    std::ifstream input(path, std::ios::binary);
    if (!input)
        throw std::runtime_error("cannot read image '" + path.string() + "'");
    input.seekg(0, std::ios::end);
    auto size = input.tellg();
    if (size < 0 || size > 128 * 1024 * 1024)
        throw std::runtime_error("image exceeds 128 MiB");
    input.seekg(0);
    std::vector<unsigned char> data(static_cast<std::size_t>(size));
    if (!input.read(reinterpret_cast<char *>(data.data()), size))
        throw std::runtime_error("image read failure");
    if (decodeRadiance(data, width_, height_, pixels_)) {
        buildMips();
        return;
    }
    auto byte = [&](std::size_t i) -> unsigned {
        if (i >= data.size())
            throw std::runtime_error("truncated image");
        return data[i];
    };
    auto le16 = [&](std::size_t i) { return byte(i) | (byte(i + 1) << 8); };
    auto le32 = [&](std::size_t i) {
        return byte(i) | (byte(i + 1) << 8) | (byte(i + 2) << 16) | (byte(i + 3) << 24);
    };
    auto allocate = [&](int w, int h) {
        width_ = integer(w, 1, 8192, "image width");
        height_ = integer(h, 1, 8192, "image height");
        if (static_cast<long long>(w) * h > 32000000)
            throw std::runtime_error("image too large");
        pixels_.resize(static_cast<std::size_t>(w) * static_cast<std::size_t>(h));
    };
    auto store = [&](std::size_t index, Vec3 c) {
        pixels_.at(index) = linearize ? Vec3{srgbDecode(c.x), srgbDecode(c.y), srgbDecode(c.z)} : c;
    };
    if (data.size() >= 2 && data[0] == 'P' && (data[1] == '3' || data[1] == '6')) {
        bool binary = data[1] == '6';
        std::size_t cursor = 2;
        auto token = [&]() {
            while (cursor < data.size()) {
                if (data[cursor] == '#') {
                    while (cursor < data.size() && data[cursor] != '\n')
                        ++cursor;
                } else if (std::isspace(data[cursor]))
                    ++cursor;
                else
                    break;
            }
            auto begin = cursor;
            while (cursor < data.size() && !std::isspace(data[cursor]) && data[cursor] != '#')
                ++cursor;
            if (begin == cursor)
                throw std::runtime_error("truncated PPM header/data");
            return std::string(data.begin() + static_cast<std::ptrdiff_t>(begin),
                               data.begin() + static_cast<std::ptrdiff_t>(cursor));
        };
        auto number = [&]() {
            auto t = token();
            std::size_t used = 0;
            double v = std::stod(t, &used);
            if (used != t.size())
                throw std::runtime_error("invalid PPM integer");
            return integer(v, 0, 65535, "PPM number");
        };
        int width = number(), height = number(), maximum = number();
        if (maximum < 1)
            throw std::runtime_error("PPM maximum is zero");
        allocate(width, height);
        if (binary) {
            if (cursor >= data.size() || !std::isspace(data[cursor]))
                throw std::runtime_error("missing P6 separator");
            unsigned sep = byte(cursor++);
            if (sep == '\r' && cursor < data.size() && data[cursor] == '\n' &&
                data.size() - cursor == pixels_.size() * 3 * (maximum > 255 ? 2u : 1u) + 1)
                ++cursor;
        }
        auto channel = [&]() {
            unsigned v = 0;
            if (binary) {
                v = byte(cursor++);
                if (maximum > 255)
                    v = v * 256 + byte(cursor++);
            } else
                v = static_cast<unsigned>(number());
            if (v > static_cast<unsigned>(maximum))
                throw std::runtime_error("PPM sample exceeds maximum");
            return double(v) / maximum;
        };
        for (std::size_t i = 0; i < pixels_.size(); ++i) {
            double r = channel(), g = channel(), b = channel();
            store(i, {r, g, b});
        }
        if (!binary) {
            while (cursor < data.size()) {
                if (data[cursor] == '#') {
                    while (cursor < data.size() && data[cursor] != '\n')
                        ++cursor;
                } else if (std::isspace(data[cursor]))
                    ++cursor;
                else
                    throw std::runtime_error("trailing P3 data");
            }
        }
        if (binary && cursor != data.size())
            throw std::runtime_error("trailing P6 data");
    } else if (data.size() >= 2 && data[0] == 'B' && data[1] == 'M') {
        if (le32(14) < 40 || le16(26) != 1 || le32(30) != 0)
            throw std::runtime_error("BMP requires uncompressed RGB DIB40+");
        std::size_t dib = le32(14), pixelOffset = le32(10);
        if (dib > data.size() - 14 || pixelOffset < 14 + dib || pixelOffset > data.size())
            throw std::runtime_error("invalid BMP raster offset/DIB size");
        int width = static_cast<int>(le32(18));
        auto rawHeight = static_cast<std::int32_t>(le32(22));
        if (rawHeight == std::numeric_limits<std::int32_t>::min())
            throw std::runtime_error("invalid BMP height");
        int height = std::abs(rawHeight);
        unsigned bits = le16(28);
        if (bits != 24 && bits != 32)
            throw std::runtime_error("BMP requires 24/32 bits");
        allocate(width, height);
        std::size_t offset = le32(10),
                    stride = ((static_cast<std::size_t>(width) * bits + 31) / 32) * 4;
        for (int y = 0; y < height; ++y)
            for (int x = 0; x < width; ++x) {
                std::size_t index = offset + static_cast<std::size_t>(y) * stride +
                                    static_cast<std::size_t>(x) * (bits / 8);
                Vec3 c{byte(index + 2) / 255., byte(index + 1) / 255., byte(index) / 255.};
                int row = rawHeight < 0 ? y : height - 1 - y;
                store(static_cast<std::size_t>(row) * static_cast<std::size_t>(width) +
                          static_cast<std::size_t>(x),
                      c);
            }
    } else {
        unsigned type = byte(2), bits = byte(16);
        if (byte(1) != 0 || (type != 2 && type != 10) || (bits != 24 && bits != 32))
            throw std::runtime_error("supported images: P3/P6 PPM, RGB BMP, true-color TGA");
        allocate(static_cast<int>(le16(12)), static_cast<int>(le16(14)));
        std::size_t cursor = 18 + byte(0), pixel = 0;
        bool top = (byte(17) & 32) != 0, right = (byte(17) & 16) != 0;
        auto color = [&]() {
            Vec3 c{byte(cursor + 2) / 255., byte(cursor + 1) / 255., byte(cursor) / 255.};
            cursor += bits / 8;
            return c;
        };
        auto write = [&](Vec3 c) {
            if (pixel >= pixels_.size())
                throw std::runtime_error("TGA RLE exceeds dimensions");
            int y = static_cast<int>(pixel / static_cast<std::size_t>(width_)),
                x = static_cast<int>(pixel % static_cast<std::size_t>(width_));
            if (!top)
                y = height_ - 1 - y;
            if (right)
                x = width_ - 1 - x;
            store(static_cast<std::size_t>(y) * static_cast<std::size_t>(width_) +
                      static_cast<std::size_t>(x),
                  c);
            ++pixel;
        };
        while (pixel < pixels_.size()) {
            unsigned packet = type == 10 ? byte(cursor++) : 0,
                     count = type == 10 ? (packet & 127) + 1 : 1;
            if (packet & 128) {
                Vec3 c = color();
                for (unsigned i = 0; i < count; ++i)
                    write(c);
            } else
                for (unsigned i = 0; i < count; ++i)
                    write(color());
        }
    }
    buildMips();
}

int Image::width() const {
    return width_;
}

int Image::height() const {
    return height_;
}

} // namespace rt
