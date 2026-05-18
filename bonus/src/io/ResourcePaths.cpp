#include "rt/io/ResourcePaths.hpp"
#include <cstdlib>
#include <stdexcept>
#ifdef __APPLE__
#include <cstdint>
#include <mach-o/dyld.h>
#include <vector>
#endif

namespace rt {
namespace {
bool resourcesAt(const std::filesystem::path &root) {
    std::error_code error;
    return std::filesystem::is_directory(root / "bonus/scenes", error) &&
           std::filesystem::is_directory(root / "bonus/assets", error);
}

std::filesystem::path executable() {
    std::error_code error;
#ifdef __APPLE__
    std::uint32_t size = 1024;
    std::vector<char> buffer(size);
    if (_NSGetExecutablePath(buffer.data(), &size) != 0) {
        buffer.resize(size);
        if (_NSGetExecutablePath(buffer.data(), &size) != 0)
            return {};
    }
    auto path = std::filesystem::canonical(buffer.data(), error);
#else
    auto path = std::filesystem::read_symlink("/proc/self/exe", error);
#endif
    return error ? std::filesystem::path{} : path;
}
} // namespace

std::filesystem::path resourceRoot() {
    if (const char *overrideRoot = std::getenv("RAYTRACER_RESOURCE_ROOT");
        overrideRoot && *overrideRoot) {
        const auto root = std::filesystem::absolute(overrideRoot);
        if (!resourcesAt(root))
            throw std::runtime_error("RAYTRACER_RESOURCE_ROOT needs bonus/scenes and bonus/assets");
        return std::filesystem::canonical(root);
    }
    const auto binary = executable();
    if (!binary.empty()) {
        const auto folder = binary.parent_path();
        for (const auto &root : {folder, folder.parent_path() / "Resources"})
            if (resourcesAt(root))
                return std::filesystem::canonical(root);
    }
    if (resourcesAt(RT_PROJECT_ROOT))
        return std::filesystem::canonical(RT_PROJECT_ROOT);
    throw std::runtime_error("cannot locate Raytracer resources; keep bonus/ beside the binary "
                             "or set RAYTRACER_RESOURCE_ROOT");
}
} // namespace rt
