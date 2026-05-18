#pragma once
#include <filesystem>

namespace rt {
// Locate bundled examples and assets independently of the current working directory.
// RAYTRACER_RESOURCE_ROOT overrides installed resources; source builds also have a fallback.
std::filesystem::path resourceRoot();
} // namespace rt
