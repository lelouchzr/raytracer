#pragma once
#include "rt/json.hpp"

namespace rt {
class JsonParser {
    const std::string &s;
    std::size_t p = 0;
    int depth = 0;
    [[noreturn]] void fail(const std::string &why) const;

    void space();

    bool take(char c);

    std::string text();

    Json read();

  public:
    explicit JsonParser(const std::string &text);

    Json parse();
};
} // namespace rt
