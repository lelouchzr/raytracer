#pragma once
#include <filesystem>

namespace rt {
class SharedLibrary {
    void *handle_;

  public:
    explicit SharedLibrary(const std::filesystem::path &);
    ~SharedLibrary();
    SharedLibrary(const SharedLibrary &) = delete;
    SharedLibrary &operator=(const SharedLibrary &) = delete;
    void *symbol(const char *) const;
};
} // namespace rt
