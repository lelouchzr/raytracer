#pragma once
#include "rt/engine.hpp"

namespace rt {
class StudioPresets {
  public:
    static const std::vector<std::string> &objects();
    static const std::vector<std::string> &lights();
    static Json object(const std::string &type, const std::string &material);
    static Json light(const std::string &type);
};
} // namespace rt
