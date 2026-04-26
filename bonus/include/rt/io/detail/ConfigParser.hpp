#pragma once

#include "rt/io/detail/ConfigValue.hpp"
#include <cstddef>
#include <string>

namespace rt::io {

class ConfigParser {
  public:
    explicit ConfigParser(const std::string &text);
    ConfigValue parse();

  private:
    const std::string &text_;
    std::size_t position_ = 0;
    unsigned depth_ = 0;
    std::size_t nodes_ = 0;

    [[noreturn]] void fail(const std::string &message) const;
    void whitespace();
    bool take(char character);
    void separators();
    std::string identifier();
    std::string quotedString();
    ConfigValue number();
    ConfigValue value();
    ConfigValue object(bool braced);
    ConfigValue sequence(char opening, char closing);
};

} // namespace rt::io
