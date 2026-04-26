#pragma once

#include "rt/io/detail/ConfigValue.hpp"
#include "rt/json.hpp"

namespace rt::io {

class SceneConverter {
  public:
    explicit SceneConverter(const ConfigValue &configuration);
    Json convert();

  private:
    const ConfigValue &configuration_;
    bool historical_ = false;
    Json::Object materials_;
    Json::Array objects_;

    Json camera() const;
    Json renderSettings() const;
    Json lights() const;
    std::string material(const ConfigValue &primitive);
    void historicalPrimitive(const std::string &type, const ConfigValue &primitive);
    void standardPrimitive(const std::string &type, const ConfigValue &primitive);
};

} // namespace rt::io
