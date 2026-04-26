#pragma once
#include "rt/engine.hpp"

namespace rt {
class Cluster {
  public:
    static void worker(unsigned short port, const std::filesystem::path &root,
                       const std::string &bind = "127.0.0.1");
    static void render(const std::filesystem::path &, const std::string &endpoints);
};
} // namespace rt
