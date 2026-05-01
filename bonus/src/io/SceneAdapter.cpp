#include "rt/io/SceneAdapter.hpp"
#include "rt/io/detail/ConfigParser.hpp"
#include "rt/io/detail/SceneConverter.hpp"
#include <algorithm>
#include <cctype>
#include <stdexcept>

namespace rt {

Json adaptScene(const std::string &text) {
    const auto start = std::find_if_not(
        text.begin(), text.end(), [](unsigned char character) { return std::isspace(character); });
    if (start == text.end())
        throw std::runtime_error("empty scene configuration");
    if (*start == '{' || *start == '[')
        return Json::parse(text);
    const auto parsed = io::ConfigParser(text).parse();
    return io::SceneConverter(parsed).convert();
}

} // namespace rt
