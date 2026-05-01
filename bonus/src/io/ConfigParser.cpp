#include "rt/io/detail/ConfigParser.hpp"
#include "rt/json.hpp"
#include <cctype>
#include <cmath>
#include <stdexcept>

namespace rt::io {

ConfigParser::ConfigParser(const std::string &text) : text_(text) {}

void ConfigParser::fail(const std::string &message) const {
    std::size_t line = 1;
    std::size_t column = 1;
    for (std::size_t i = 0; i < position_; ++i) {
        if (text_[i] == '\n') {
            ++line;
            column = 1;
        } else {
            ++column;
        }
    }
    throw std::runtime_error("configuration " + std::to_string(line) + ":" +
                             std::to_string(column) + ": " + message);
}

void ConfigParser::whitespace() {
    while (position_ < text_.size()) {
        if (std::isspace(static_cast<unsigned char>(text_[position_]))) {
            ++position_;
        } else if (text_[position_] == '#' || text_.compare(position_, 2, "//") == 0) {
            while (position_ < text_.size() && text_[position_] != '\n')
                ++position_;
        } else if (text_.compare(position_, 2, "/*") == 0) {
            position_ += 2;
            const auto end = text_.find("*/", position_);
            if (end == std::string::npos)
                fail("unterminated block comment");
            position_ = end + 2;
        } else {
            break;
        }
    }
}

bool ConfigParser::take(char character) {
    whitespace();
    if (position_ == text_.size() || text_[position_] != character)
        return false;
    ++position_;
    return true;
}

void ConfigParser::separators() {
    whitespace();
    while (position_ < text_.size() && (text_[position_] == ',' || text_[position_] == ';')) {
        ++position_;
        whitespace();
    }
}

std::string ConfigParser::identifier() {
    whitespace();
    if (position_ < text_.size() && text_[position_] == '"')
        return quotedString();
    const auto start = position_;
    while (position_ < text_.size()) {
        const auto character = static_cast<unsigned char>(text_[position_]);
        if (!std::isalnum(character) && character != '_' && character != '-')
            break;
        ++position_;
    }
    if (start == position_)
        fail("expected field name");
    return text_.substr(start, position_ - start);
}

std::string ConfigParser::quotedString() {
    whitespace();
    const auto start = position_++;
    bool escaped = false;
    while (position_ < text_.size()) {
        const char character = text_[position_++];
        if (!escaped && character == '"')
            return Json::parse(text_.substr(start, position_ - start)).string();
        if (!escaped && character == '\\')
            escaped = true;
        else
            escaped = false;
    }
    fail("unterminated string");
}

ConfigValue ConfigParser::number() {
    const auto start = position_;
    if (position_ < text_.size() && (text_[position_] == '+' || text_[position_] == '-'))
        ++position_;
    while (position_ < text_.size()) {
        const char character = text_[position_];
        if (!std::isdigit(static_cast<unsigned char>(character)) && character != '.' &&
            character != 'e' && character != 'E' && character != '+' && character != '-')
            break;
        ++position_;
    }
    const auto token = text_.substr(start, position_ - start);
    std::size_t consumed = 0;
    ConfigValue result;
    result.kind = ConfigKind::Number;
    try {
        result.number = std::stod(token, &consumed);
    } catch (const std::exception &) {
        fail("invalid number");
    }
    if (consumed != token.size() || !std::isfinite(result.number))
        fail("invalid or non-finite number");
    return result;
}

ConfigValue ConfigParser::sequence(char opening, char closing) {
    if (!take(opening))
        fail("expected sequence");
    ConfigValue result;
    result.kind = ConfigKind::Array;
    separators();
    while (!take(closing)) {
        result.elements.push_back(value());
        whitespace();
        if (position_ < text_.size() && text_[position_] != closing && text_[position_] != ',' &&
            text_[position_] != ';')
            fail("expected sequence separator");
        separators();
    }
    return result;
}

ConfigValue ConfigParser::object(bool braced) {
    if (braced && !take('{'))
        fail("expected '{'");
    ConfigValue result;
    separators();
    while (position_ < text_.size()) {
        if (braced && take('}'))
            return result;
        // The historical plane format has three adjacent coefficient groups.
        if (text_[position_] == '{') {
            if (result.members.empty())
                fail("unnamed group without a preceding field");
            const auto name = result.members.back().first;
            result.members.emplace_back(name, value());
        } else {
            auto name = identifier();
            const bool assignment = take(':') || take('=');
            whitespace();
            if (!assignment && (position_ == text_.size() || text_[position_] != '{'))
                fail("expected ':', '=' or group");
            result.members.emplace_back(std::move(name), value());
        }
        separators();
    }
    if (braced)
        fail("unterminated group");
    return result;
}

ConfigValue ConfigParser::value() {
    whitespace();
    if (++nodes_ > 250000)
        fail("configuration exceeds 250000 nodes");
    if (position_ == text_.size())
        fail("expected value");
    if (++depth_ > 128)
        fail("nesting limit exceeded");
    ConfigValue result;
    const char character = text_[position_];
    if (character == '{') {
        result = object(true);
    } else if (character == '(' || character == '[') {
        result = sequence(character, character == '(' ? ')' : ']');
    } else if (character == '"') {
        result.kind = ConfigKind::String;
        result.text = quotedString();
    } else if (std::isalpha(static_cast<unsigned char>(character))) {
        result.kind = ConfigKind::Boolean;
        result.text = identifier();
        if (result.text != "true" && result.text != "false")
            fail("unquoted string or unsupported literal");
    } else {
        result = number();
    }
    --depth_;
    return result;
}

ConfigValue ConfigParser::parse() {
    if (text_.size() > 16 * 1024 * 1024)
        fail("configuration exceeds 16 MiB");
    return object(false);
}

} // namespace rt::io
