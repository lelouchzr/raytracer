#include "rt/studio/StudioDocument.hpp"
#include "rt/io/SceneAdapter.hpp"
#include <chrono>
#include <fstream>
#include <functional>
#include <sstream>

namespace rt {
void StudioDocument::resolveAssets(Json &json, const std::filesystem::path &base) {
    if (auto object = std::get_if<Json::Object>(&json.value)) {
        for (auto &[key, child] : *object) {
            if ((key == "file" || key == "normalFile") &&
                std::holds_alternative<std::string>(child.value))
                child = Json(std::filesystem::weakly_canonical(base / child.string()).string());
            else if ((key == "plugins" || key == "include")) {
                for (auto &path : std::get<Json::Array>(child.value))
                    path = Json(std::filesystem::weakly_canonical(base / path.string()).string());
            } else
                resolveAssets(child, base);
        }
    } else if (auto array = std::get_if<Json::Array>(&json.value))
        for (auto &child : *array)
            resolveAssets(child, base);
}

Json StudioDocument::read(const std::filesystem::path &path,
                          std::vector<std::filesystem::path> &stack) {
    auto canonical = std::filesystem::canonical(path);
    if (stack.size() >= 32 || std::find(stack.begin(), stack.end(), canonical) != stack.end())
        throw std::runtime_error("scene import cycle/depth limit");
    stack.push_back(canonical);
    std::ifstream input(canonical);
    input.seekg(0, std::ios::end);
    if (!input || input.tellg() > 16 * 1024 * 1024)
        throw std::runtime_error("cannot read scene or scene exceeds 16 MiB");
    input.seekg(0);
    std::ostringstream text;
    text << input.rdbuf();
    Json json = adaptScene(text.str());
    Json::Object merged;
    auto combine = [&](const Json &part) {
        for (const auto &[key, value] : part.object()) {
            if (key == "include")
                continue;
            if (key == "objects" || key == "lights" || key == "plugins") {
                if (!merged.count(key))
                    merged[key] = Json(Json::Array{});
                auto &array = std::get<Json::Array>(merged[key].value);
                array.insert(array.end(), value.array().begin(), value.array().end());
            } else if (key == "materials") {
                if (!merged.count(key))
                    merged[key] = Json(Json::Object{});
                auto &materials = std::get<Json::Object>(merged[key].value);
                for (const auto &[name, material] : value.object())
                    if (!materials.emplace(name, material).second)
                        throw std::runtime_error("duplicate material '" + name + "'");
            } else
                merged[key] = value;
        }
    };
    if (json.has("include"))
        for (const auto &child : json.at("include").array())
            combine(read(canonical.parent_path() / child.string(), stack));
    resolveAssets(json, canonical.parent_path());
    combine(json);
    stack.pop_back();
    if (!merged.count("lights"))
        merged["lights"] = Json(Json::Array{});
    if (!merged.count("render"))
        merged["render"] = Json(Json::Object{});
    return Json(std::move(merged));
}

StudioDocument::StudioDocument(const std::filesystem::path &source)
    : source_(std::filesystem::canonical(source)) {
    std::vector<std::filesystem::path> stack;
    replace(read(source_, stack));
}

Json &StudioDocument::data() {
    return document_;
}

const Json &StudioDocument::data() const {
    return document_;
}

const std::filesystem::path &StudioDocument::source() const {
    return source_;
}

void StudioDocument::replace(Json json) {
    if (!json.has("camera") || !json.has("materials") || !json.has("objects") ||
        json.at("objects").array().empty())
        throw std::runtime_error("scene requires camera, materials and objects");
    json.at("camera").object();
    json.at("materials").object();
    std::function<void(const Json &)> inspectObject = [&](const Json &object) {
        auto type = object.at("type").string();
        if (type != "group")
            object.at("material").string();
        if (object.has("translate"))
            object.at("translate").vector();
        if (object.has("children"))
            for (const auto &child : object.at("children").array())
                inspectObject(child);
    };
    for (const auto &object : json.at("objects").array())
        inspectObject(object);
    for (const auto &[name, material] : json.at("materials").object()) {
        (void)name;
        material.object();
    }
    if (json.has("lights"))
        for (const auto &light : json.at("lights").array())
            light.at("type").string();
    if (json.has("render"))
        json.at("render").object();
    resolveAssets(json, source_.parent_path());
    if (!json.has("lights"))
        std::get<Json::Object>(json.value)["lights"] = Json(Json::Array{});
    if (!json.has("render"))
        std::get<Json::Object>(json.value)["render"] = Json(Json::Object{});
    document_ = std::move(json);
}

std::string StudioDocument::formatted(const Json &json) {
    std::string result;
    std::function<void(const Json &, int)> visit = [&](const Json &value, int depth) {
        const auto *object = std::get_if<Json::Object>(&value.value);
        const auto *array = std::get_if<Json::Array>(&value.value);
        if (!object && !array) {
            result += value.dump();
            return;
        }
        result += object ? '{' : '[';
        std::size_t index = 0, size = object ? object->size() : array->size();
        auto item = [&](const Json &child, const std::string &key) {
            result += '\n' + std::string(static_cast<std::size_t>(depth + 1) * 2, ' ');
            if (object)
                result += Json(key).dump() + ": ";
            visit(child, depth + 1);
            if (++index < size)
                result += ',';
        };
        if (object)
            for (const auto &[key, child] : *object)
                item(child, key);
        else
            for (const auto &child : *array)
                item(child, "");
        if (size)
            result += '\n' + std::string(static_cast<std::size_t>(depth) * 2, ' ');
        result += object ? '}' : ']';
    };
    visit(json, 0);
    return result;
}

void StudioDocument::write(const Json &json, const std::filesystem::path &path) {
    auto temporary = path.string() + ".tmp." +
                     std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    try {
        std::ofstream output(temporary);
        if (!output)
            throw std::runtime_error("cannot save scene '" + path.string() + "'");
        output << formatted(json) << '\n';
        output.flush();
        if (!output)
            throw std::runtime_error("scene write failed");
        output.close();
        if (!output)
            throw std::runtime_error("scene close failed");
        std::filesystem::rename(temporary, path);
    } catch (...) {
        std::error_code error;
        std::filesystem::remove(temporary, error);
        throw;
    }
}

void StudioDocument::save(const std::filesystem::path &path) {
    write(document_, path);
    source_ = std::filesystem::absolute(path);
}
} // namespace rt
