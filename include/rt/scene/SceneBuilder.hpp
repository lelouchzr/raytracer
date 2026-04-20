#pragma once
#include "rt/scene/Factory.hpp"
#include "rt/scene/Scene.hpp"

namespace rt {
class SceneBuilder {
    [[maybe_unused]] bool preview_, deferLighting_;
    Factory factory_;
    Scene scene_;
    std::map<std::string, std::shared_ptr<const IMaterial>> materials_;
    void append(const std::filesystem::path &, std::vector<std::filesystem::path> &, bool root);

  public:
    explicit SceneBuilder(bool preview = false, bool deferLighting = false);
    Scene build(const std::filesystem::path &);
};
} // namespace rt
