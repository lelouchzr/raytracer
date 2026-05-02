#include "rt/plugins/SharedLibrary.hpp"
#include <dlfcn.h>
#include <stdexcept>

namespace rt {
SharedLibrary::SharedLibrary(const std::filesystem::path &path) : handle_(nullptr) {
    dlerror();
    handle_ = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (!handle_) {
        const char *error = dlerror();
        throw std::runtime_error("plugin '" + path.string() +
                                 "': " + (error ? error : "dlopen failed"));
    }
}

SharedLibrary::~SharedLibrary() {
    if (handle_)
        dlclose(handle_);
}

void *SharedLibrary::symbol(const char *name) const {
    dlerror();
    void *result = dlsym(handle_, name);
    const char *error = dlerror();
    if (error)
        throw std::runtime_error(std::string("plugin symbol '") + name + "': " + error);
    return result;
}
} // namespace rt
