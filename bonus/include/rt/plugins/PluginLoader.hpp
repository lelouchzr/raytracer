#pragma once
#include "rt/engine.hpp"

namespace rt {
class PluginLoader {
  public:
    static void load(Factory &, const std::filesystem::path &);
};
} // namespace rt
