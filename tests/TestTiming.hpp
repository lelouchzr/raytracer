#pragma once
#include <chrono>
#include <cstdlib>
#include <string_view>

namespace rt::test {
inline std::chrono::seconds budget(int seconds) {
    const char *mode = std::getenv("EPITECH_MEMCHECK");
    // Preserve normal latency limits; instrumentation slows allocations and worker execution.
    const int scale = mode && std::string_view(mode) == "1" ? 10 : 1;
    return std::chrono::seconds(seconds * scale);
}
} // namespace rt::test
