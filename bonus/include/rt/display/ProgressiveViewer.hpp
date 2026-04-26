#pragma once
#include <filesystem>

namespace rt {
class ProgressiveViewer {
  public:
    static void run(const std::filesystem::path &, bool watch, bool preview, int frameLimit = 0);
};
} // namespace rt
