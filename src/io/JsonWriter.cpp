#include "rt/io/Json.hpp"
#include <iomanip>
#include <sstream>

namespace rt {
std::string Json::dump() const {
    if (std::holds_alternative<std::nullptr_t>(value))
        return "null";
    if (auto flag = std::get_if<bool>(&value))
        return *flag ? "true" : "false";
    if (auto number = std::get_if<double>(&value)) {
        if (!std::isfinite(*number))
            throw std::runtime_error("cannot serialize non-finite JSON number");
        std::ostringstream out;
        out << std::setprecision(std::numeric_limits<double>::max_digits10) << *number;
        return out.str();
    }
    if (auto text = std::get_if<std::string>(&value)) {
        std::string out = "\"";
        const char *hex = "0123456789abcdef";
        for (char raw : *text) {
            auto c = static_cast<unsigned char>(raw);
            if (c == '"' || c == '\\') {
                out += '\\';
                out += static_cast<char>(c);
            } else if (c < 32) {
                out += "\\u00";
                out += hex[c / 16];
                out += hex[c % 16];
            } else
                out += static_cast<char>(c);
        }
        return out + '"';
    }
    std::string out;
    if (auto array = std::get_if<Array>(&value)) {
        out = "[";
        for (const auto &item : *array) {
            if (out.size() > 1)
                out += ',';
            out += item.dump();
        }
        return out + ']';
    }
    out = "{";
    for (const auto &[key, item] : std::get<Object>(value)) {
        if (out.size() > 1)
            out += ',';
        out += Json(key).dump() + ':' + item.dump();
    }
    return out + '}';
}
} // namespace rt
