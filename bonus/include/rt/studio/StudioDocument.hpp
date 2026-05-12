#pragma once
#include "rt/engine.hpp"

namespace rt {
class StudioDocument {
    Json document_;
    std::filesystem::path source_;
    static Json read(const std::filesystem::path &, std::vector<std::filesystem::path> &);
    static void resolveAssets(Json &, const std::filesystem::path &);

  public:
    explicit StudioDocument(const std::filesystem::path &);
    Json &data();
    const Json &data() const;
    const std::filesystem::path &source() const;
    void replace(Json);
    void save(const std::filesystem::path &);
    static void write(const Json &, const std::filesystem::path &);
    static std::string formatted(const Json &);
};
} // namespace rt
