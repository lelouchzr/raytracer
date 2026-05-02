#include "rt/network/AssetScope.hpp"
#include "rt/io/SceneAdapter.hpp"
#include <fstream>
#include <functional>
#include <sstream>

namespace rt {
AssetScope::AssetScope(const std::filesystem::path &root)
    : root_(std::filesystem::canonical(root)) {}

std::filesystem::path AssetScope::confined(const std::filesystem::path &file) const {
    auto canonical = std::filesystem::canonical(file);
    auto relative = canonical.lexically_relative(root_);
    if (relative.empty() || *relative.begin() == "..")
        throw std::runtime_error("worker asset escapes root: " + file.string());
    return canonical;
}

void AssetScope::inspect(const std::filesystem::path &file, int depth, Role role) {
    if (depth > 32)
        throw std::runtime_error("worker asset dependency depth exceeded");
    auto canonical = confined(file);
    if (!visited_.insert({canonical, role}).second)
        return;
    std::ifstream input(canonical, std::ios::binary);
    input.seekg(0, std::ios::end);
    auto size = input.tellg();
    if (size < 0 || size > 128 * 1024 * 1024)
        throw std::runtime_error("worker asset exceeds size limit");
    input.seekg(0);
    if (role == Role::Leaf)
        return;
    if (role == Role::Mesh || role == Role::Material) {
        std::string line;
        while (std::getline(input, line)) {
            std::istringstream row(line);
            std::string key;
            row >> key;
            if (key == "mtllib") {
                std::string name;
                while (row >> name)
                    inspect(canonical.parent_path() / name, depth + 1,
                            key == "mtllib" ? Role::Material : Role::Leaf);
            } else if (key == "map_Kd" || key == "map_Bump" || key == "bump" || key == "norm") {
                std::string name;
                std::getline(row >> std::ws, name);
                inspect(canonical.parent_path() / name, depth + 1,
                        key == "mtllib" ? Role::Material : Role::Leaf);
            }
        }
        return;
    }
    std::ostringstream text;
    text << input.rdbuf();
    Json json = adaptScene(text.str());
    std::function<void(const Json &)> walk = [&](const Json &value) {
        if (auto object = std::get_if<Json::Object>(&value.value)) {
            for (const auto &[key, child] : *object) {
                if ((key == "file" || key == "normalFile") &&
                    std::holds_alternative<std::string>(child.value))
                    inspect(canonical.parent_path() / child.string(), depth + 1,
                            key == "file" && object->count("type") &&
                                    object->at("type").string() == "obj"
                                ? Role::Mesh
                                : Role::Leaf);
                else if (key == "include" || key == "plugins") {
                    for (const auto &path : child.array())
                        inspect(canonical.parent_path() / path.string(), depth + 1,
                                key == "include" ? Role::Scene : Role::Leaf);
                } else
                    walk(child);
            }
        } else if (auto array = std::get_if<Json::Array>(&value.value))
            for (const auto &item : *array)
                walk(item);
    };
    walk(json);
}

void AssetScope::validate(const std::filesystem::path &file) {
    visited_.clear();
    inspect(file, 0, Role::Scene);
}
} // namespace rt
