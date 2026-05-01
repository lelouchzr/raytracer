#include "rt/io/MeshLoader.hpp"
#include "rt/Extensions.hpp"
#include "rt/geometry/Triangle.hpp"
#include "rt/materials/AdvancedMaterial.hpp"
#include "rt/transform/AffineTransform.hpp"
#include <fstream>
#include <numeric>
#include <sstream>

namespace rt {
void MeshLoader::loadMaterials(const std::filesystem::path &path) {
    std::ifstream input(path);
    if (!input)
        throw std::runtime_error("cannot read MTL '" + path.string() + "'");
    std::string line, name;
    Json::Object definition;
    auto finish = [&]() {
        if (!name.empty())
            materials_[name] =
                std::make_shared<AdvancedMaterial>(Json(definition), path.parent_path());
    };
    while (std::getline(input, line)) {
        std::istringstream row(line);
        std::string key;
        row >> key;
        if (key == "newmtl") {
            finish();
            row >> name;
            definition = {{"type", Json(std::string("phong"))}};
        } else if (!name.empty() && (key == "Kd" || key == "Ke")) {
            Vec3 c;
            if (!(row >> c.x >> c.y >> c.z))
                throw std::runtime_error("invalid MTL color");
            definition[key == "Kd" ? "color" : "emission"] =
                Json(Json::Array{Json(c.x), Json(c.y), Json(c.z)});
        } else if (!name.empty() && (key == "Ns" || key == "Ni" || key == "d" || key == "Tr")) {
            double v;
            if (!(row >> v))
                throw std::runtime_error("invalid MTL coefficient");
            if (key == "Ns")
                definition["shininess"] = Json(std::clamp(v, 1., 2048.));
            else if (key == "Ni")
                definition["ior"] = Json(v);
            else
                definition["transmission"] = Json(key == "d" ? 1 - v : v);
        } else if (!name.empty() &&
                   (key == "map_Kd" || key == "map_Bump" || key == "bump" || key == "norm")) {
            std::string file;
            std::getline(row >> std::ws, file);
            if (file.empty() || file[0] == '-')
                throw std::runtime_error(
                    "MTL texture options unsupported; provide a plain filename");
            if (key == "map_Kd") {
                definition["pattern"] = Json(std::string("image"));
                definition["file"] = Json(file);
            } else
                definition["normalFile"] = Json(file);
        }
    }
    finish();
}

Primitive MeshLoader::load(const Json &j, std::shared_ptr<const IMaterial> fallback,
                           const std::filesystem::path &base) {
    vertices_.clear();
    normals_.clear();
    uvs_.clear();
    materials_.clear();
    keys(j, {"type", "file", "material", "translate", "rotate", "scale", "shear", "matrix"});
    std::filesystem::path path = base / j.at("file").string();
    std::ifstream input(path);
    if (!input)
        throw std::runtime_error("cannot read OBJ '" + path.string() + "'");
    std::vector<Primitive> triangles;
    auto current = fallback;
    std::string line;
    auto resolve = [](const std::string &token, std::size_t size) {
        std::size_t used = 0;
        long index = std::stol(token, &used);
        if (used != token.size() || index == 0)
            throw std::runtime_error("invalid OBJ index");
        long actual = index > 0 ? index - 1 : static_cast<long>(size) + index;
        if (actual < 0 || static_cast<std::size_t>(actual) >= size)
            throw std::runtime_error("OBJ index out of bounds");
        return static_cast<std::size_t>(actual);
    };
    while (std::getline(input, line)) {
        std::istringstream row(line);
        std::string key;
        row >> key;
        if (key == "v" || key == "vn" || key == "vt") {
            Vec3 v;
            if (!(row >> v.x >> v.y))
                throw std::runtime_error("invalid OBJ vector");
            if (key != "vt" && !(row >> v.z))
                throw std::runtime_error("invalid OBJ vector");
            for (int i = 0; i < 3; ++i)
                range(v[i], -1e9, 1e9, "OBJ coordinate");
            (key == "v"    ? vertices_
             : key == "vn" ? normals_
                           : uvs_)
                .push_back(key == "vn" ? unit(v) : v);
        } else if (key == "mtllib") {
            std::string file;
            while (row >> file)
                loadMaterials(path.parent_path() / file);
        } else if (key == "usemtl") {
            std::string name;
            row >> name;
            auto it = materials_.find(name);
            if (it == materials_.end())
                throw std::runtime_error("unknown OBJ material '" + name + "'");
            current = it->second;
        } else if (key == "f") {
            std::vector<Vec3> positions, normals, uvs;
            bool smooth = true, textured = true;
            std::string token;
            while (row >> token) {
                if (token[0] == '#')
                    break;
                auto first = token.find('/'),
                     second = first == std::string::npos ? first : token.find('/', first + 1);
                positions.push_back(
                    vertices_.at(resolve(token.substr(0, first), vertices_.size())));
                std::string uv = first == std::string::npos
                                     ? ""
                                     : token.substr(first + 1, second == std::string::npos
                                                                   ? std::string::npos
                                                                   : second - first - 1),
                            normal = second == std::string::npos ? "" : token.substr(second + 1);
                textured = textured && !uv.empty();
                smooth = smooth && !normal.empty();
                uvs.push_back(uv.empty() ? Vec3{} : uvs_.at(resolve(uv, uvs_.size())));
                normals.push_back(normal.empty() ? Vec3{}
                                                 : normals_.at(resolve(normal, normals_.size())));
            }
            if (positions.size() < 3 || positions.size() > 4096)
                throw std::runtime_error("OBJ face requires 3..4096 vertices");
            // Project onto the dominant normal axis, then ear-clip concave simple polygons.
            Vec3 normal;
            for (std::size_t i = 0; i < positions.size(); ++i) {
                Vec3 a = positions[i], b = positions[(i + 1) % positions.size()];
                normal += {(a.y - b.y) * (a.z + b.z), (a.z - b.z) * (a.x + b.x),
                           (a.x - b.x) * (a.y + b.y)};
            }
            normal = unit(normal);
            int axis = std::abs(normal.x) > std::abs(normal.y)
                           ? (std::abs(normal.x) > std::abs(normal.z) ? 0 : 2)
                           : (std::abs(normal.y) > std::abs(normal.z) ? 1 : 2);
            auto project = [&](Vec3 p) {
                return axis == 0   ? Vec3{p.y, p.z, 0}
                       : axis == 1 ? Vec3{p.x, p.z, 0}
                                   : Vec3{p.x, p.y, 0};
            };
            auto orientation = [](Vec3 a, Vec3 b, Vec3 c) {
                return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
            };
            double area = 0;
            for (std::size_t i = 0; i < positions.size(); ++i) {
                Vec3 a = project(positions[i]), b = project(positions[(i + 1) % positions.size()]);
                area += a.x * b.y - a.y * b.x;
            }
            double sign = area > 0 ? 1 : -1;
            std::vector<std::size_t> remaining(positions.size());
            std::iota(remaining.begin(), remaining.end(), 0);
            while (remaining.size() > 2) {
                bool clipped = false;
                for (std::size_t i = 0; i < remaining.size(); ++i) {
                    std::size_t a = remaining[(i + remaining.size() - 1) % remaining.size()],
                                b = remaining[i], c = remaining[(i + 1) % remaining.size()];
                    Vec3 p = project(positions[a]), q = project(positions[b]),
                         r = project(positions[c]);
                    if (orientation(p, q, r) * sign < 1e-12)
                        continue;
                    bool inside = false;
                    for (auto index : remaining) {
                        if (index == a || index == b || index == c)
                            continue;
                        Vec3 v = project(positions[index]);
                        if (orientation(p, q, v) * sign >= 0 && orientation(q, r, v) * sign >= 0 &&
                            orientation(r, p, v) * sign >= 0) {
                            inside = true;
                            break;
                        }
                    }
                    if (inside)
                        continue;
                    triangles.push_back(std::make_shared<Triangle>(
                        std::array<Vec3, 3>{positions[a], positions[b], positions[c]},
                        std::array<Vec3, 3>{normals[a], normals[b], normals[c]},
                        std::array<Vec3, 3>{uvs[a], uvs[b], uvs[c]}, smooth, textured, current));
                    remaining.erase(remaining.begin() + static_cast<std::ptrdiff_t>(i));
                    clipped = true;
                    break;
                }
                if (!clipped)
                    throw std::runtime_error("OBJ face is degenerate, non-simple or non-planar");
            }
        }
        if (vertices_.size() > 1000000 || triangles.size() > 1000000)
            throw std::runtime_error("OBJ geometry limit exceeded");
    }
    if (triangles.empty())
        throw std::runtime_error("OBJ contains no faces");
    return applyAffine(buildBvh(std::move(triangles)), j);
}
} // namespace rt

namespace rt {
Primitive makeMesh(const Json &j, std::shared_ptr<const IMaterial> material,
                   const std::filesystem::path &base) {
    return MeshLoader().load(j, std::move(material), base);
}

} // namespace rt
