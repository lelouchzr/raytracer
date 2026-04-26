#pragma once
#include "rt/engine.hpp"

namespace rt {
class MeshLoader {
    std::vector<Vec3> vertices_, normals_, uvs_;
    std::map<std::string, std::shared_ptr<const IMaterial>> materials_;
    void loadMaterials(const std::filesystem::path &);

  public:
    Primitive load(const Json &, std::shared_ptr<const IMaterial>, const std::filesystem::path &);
};
} // namespace rt
