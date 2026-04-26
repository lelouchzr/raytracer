#pragma once
#include <filesystem>
#include <set>

namespace rt {
class AssetScope {
    std::filesystem::path root_;
    enum class Role { Scene, Mesh, Material, Leaf };
    std::set<std::pair<std::filesystem::path, Role>> visited_;
    std::filesystem::path confined(const std::filesystem::path &) const;
    void inspect(const std::filesystem::path &, int, Role);

  public:
    explicit AssetScope(const std::filesystem::path &);
    void validate(const std::filesystem::path &);
};
} // namespace rt
