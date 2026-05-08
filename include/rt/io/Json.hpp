#pragma once
#include "rt/math.hpp"
#include <map>
#include <string>
#include <variant>
#include <vector>

namespace rt {
class Json {
  public:
    using Object = std::map<std::string, Json>;
    using Array = std::vector<Json>;
    std::variant<std::nullptr_t, bool, double, std::string, Array, Object> value = nullptr;
    Json();
    explicit Json(bool);
    explicit Json(double);
    explicit Json(std::string);
    explicit Json(Array);
    explicit Json(Object);
    static Json parse(const std::string &text);
    std::string dump() const;
    const Object &object() const;
    const Array &array() const;
    const Json &at(const std::string &key) const;
    bool has(const std::string &key) const;
    double number() const;
    std::string string() const;
    double number(const std::string &key, double fallback) const;
    std::string string(const std::string &key, const std::string &fallback) const;
    Vec3 vector() const;
    Vec3 vector(const std::string &key, Vec3 fallback) const;
};

double range(double v, double lo, double hi, const std::string &name);
int integer(double v, int lo, int hi, const std::string &name);
void keys(const Json &j, std::initializer_list<const char *> allowed);
} // namespace rt
