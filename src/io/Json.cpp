#include "rt/json.hpp"
#include "rt/io/JsonParser.hpp"
#include <cctype>
#include <charconv>
#include <set>

namespace rt {
Json::Json() = default;

Json::Json(bool v) : value(v) {}

Json::Json(double v) : value(v) {}

Json::Json(std::string v) : value(std::move(v)) {}

Json::Json(Array v) : value(std::move(v)) {}

Json::Json(Object v) : value(std::move(v)) {}

Json Json::parse(const std::string &text) {
    return JsonParser(text).parse();
}

const Json::Object &Json::object() const {
    auto v = std::get_if<Object>(&value);
    if (!v)
        throw std::runtime_error("expected object");
    return *v;
}

const Json::Array &Json::array() const {
    auto v = std::get_if<Array>(&value);
    if (!v)
        throw std::runtime_error("expected array");
    return *v;
}

const Json &Json::at(const std::string &key) const {
    const auto &o = object();
    auto it = o.find(key);
    if (it == o.end())
        throw std::runtime_error("missing field '" + key + "'");
    return it->second;
}

bool Json::has(const std::string &key) const {
    return object().count(key) > 0;
}

double Json::number() const {
    auto v = std::get_if<double>(&value);
    if (!v)
        throw std::runtime_error("expected number");
    return *v;
}

std::string Json::string() const {
    auto v = std::get_if<std::string>(&value);
    if (!v)
        throw std::runtime_error("expected string");
    return *v;
}

double Json::number(const std::string &key, double fallback) const {
    return has(key) ? at(key).number() : fallback;
}

std::string Json::string(const std::string &key, const std::string &fallback) const {
    return has(key) ? at(key).string() : fallback;
}

Vec3 Json::vector() const {
    auto &a = array();
    if (a.size() != 3)
        throw std::runtime_error("expected vector [x,y,z]");
    return {range(a[0].number(), -1e9, 1e9, "vector x"),
            range(a[1].number(), -1e9, 1e9, "vector y"),
            range(a[2].number(), -1e9, 1e9, "vector z")};
}

Vec3 Json::vector(const std::string &key, Vec3 fallback) const {
    return has(key) ? at(key).vector() : fallback;
}

double range(double v, double lo, double hi, const std::string &name) {
    if (!std::isfinite(v) || v < lo || v > hi)
        throw std::runtime_error(name + " outside [" + std::to_string(lo) + ", " +
                                 std::to_string(hi) + "]");
    return v;
}

int integer(double v, int lo, int hi, const std::string &name) {
    range(v, lo, hi, name);
    if (v != std::floor(v))
        throw std::runtime_error(name + " must be an integer");
    return int(v);
}

void keys(const Json &j, std::initializer_list<const char *> allowed) {
    std::set<std::string> names;
    for (auto s : allowed)
        names.insert(s);
    for (const auto &[name, v] : j.object()) {
        (void)v;
        if (!names.count(name))
            throw std::runtime_error("unknown field '" + name + "'");
    }
}
} // namespace rt
