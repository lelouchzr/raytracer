#include "rt/io/JsonParser.hpp"
#include <cctype>

namespace rt {
[[noreturn]] void JsonParser::fail(const std::string &why) const {
    std::size_t line = 1, column = 1;
    for (std::size_t i = 0; i < p; ++i) {
        if (s[i] == '\n') {
            ++line;
            column = 1;
        } else
            ++column;
    }
    throw std::runtime_error("JSON " + std::to_string(line) + ":" + std::to_string(column) + ": " +
                             why);
}

void JsonParser::space() {
    while (p < s.size() && std::isspace(static_cast<unsigned char>(s[p])))
        ++p;
}

bool JsonParser::take(char c) {
    space();
    if (p < s.size() && s[p] == c) {
        ++p;
        return true;
    }
    return false;
}

std::string JsonParser::text() {
    if (!take('"'))
        fail("expected string");
    std::string out;
    while (p < s.size()) {
        unsigned char c = static_cast<unsigned char>(s[p++]);
        if (c == '"')
            return out;
        if (c < 32)
            fail("control character in string");
        if (c == '\\') {
            if (p == s.size())
                fail("unfinished escape");
            char e = s[p++];
            switch (e) {
            case '"':
                out += '"';
                break;
            case '\\':
                out += '\\';
                break;
            case '/':
                out += '/';
                break;
            case 'n':
                out += '\n';
                break;
            case 'r':
                out += '\r';
                break;
            case 't':
                out += '\t';
                break;
            case 'b':
                out += '\b';
                break;
            case 'f':
                out += '\f';
                break;
            case 'u': {
                unsigned code = 0;
                for (int i = 0; i < 4; ++i) {
                    if (p == s.size())
                        fail("unfinished unicode escape");
                    char h = s[p++];
                    if (!std::isxdigit(static_cast<unsigned char>(h)))
                        fail("invalid unicode escape");
                    code = code * 16 + unsigned(h <= '9' ? h - '0' : std::tolower(h) - 'a' + 10);
                }
                if (code >= 0xd800 && code <= 0xdfff)
                    fail("surrogate escape unsupported; use UTF-8");
                if (code < 128)
                    out += char(code);
                else if (code < 2048) {
                    out += char(192 | (code >> 6));
                    out += char(128 | (code & 63));
                } else {
                    out += char(224 | (code >> 12));
                    out += char(128 | ((code >> 6) & 63));
                    out += char(128 | (code & 63));
                }
                break;
            }
            default:
                fail("invalid escape");
            }
        } else
            out += char(c);
    }
    fail("unterminated string");
}

Json JsonParser::read() {
    space();
    if (p == s.size())
        fail("expected value");
    if (++depth > 128)
        fail("nesting limit exceeded");
    Json result;
    if (s[p] == '{') {
        ++p;
        Json::Object obj;
        if (!take('}')) {
            do {
                auto k = text();
                if (!take(':'))
                    fail("expected ':'");
                if (obj.count(k))
                    fail("duplicate key '" + k + "'");
                obj.emplace(k, read());
            } while (take(','));
            if (!take('}'))
                fail("expected '}'");
        }
        result = Json(std::move(obj));
    } else if (s[p] == '[') {
        ++p;
        Json::Array a;
        if (!take(']')) {
            do {
                a.push_back(read());
            } while (take(','));
            if (!take(']'))
                fail("expected ']'");
        }
        result = Json(std::move(a));
    } else if (s[p] == '"')
        result = Json(text());
    else if (s.compare(p, 4, "true") == 0) {
        p += 4;
        result = Json(true);
    } else if (s.compare(p, 5, "false") == 0) {
        p += 5;
        result = Json(false);
    } else if (s.compare(p, 4, "null") == 0) {
        p += 4;
        result = Json();
    } else {
        const auto start = p;
        if (s[p] == '-')
            ++p;
        if (p == s.size() || !std::isdigit(static_cast<unsigned char>(s[p])))
            fail("expected number");
        if (s[p] == '0')
            ++p;
        else
            while (p < s.size() && std::isdigit(static_cast<unsigned char>(s[p])))
                ++p;
        if (p < s.size() && s[p] == '.') {
            ++p;
            auto n = p;
            while (p < s.size() && std::isdigit(static_cast<unsigned char>(s[p])))
                ++p;
            if (p == n)
                fail("missing fractional digits");
        }
        if (p < s.size() && (s[p] == 'e' || s[p] == 'E')) {
            ++p;
            if (p < s.size() && (s[p] == '+' || s[p] == '-'))
                ++p;
            auto n = p;
            while (p < s.size() && std::isdigit(static_cast<unsigned char>(s[p])))
                ++p;
            if (p == n)
                fail("missing exponent");
        }
        double number = 0;
        try {
            number = std::stod(s.substr(start, p - start));
        } catch (...) {
            fail("number out of range");
        }
        if (!std::isfinite(number))
            fail("non-finite number");
        result = Json(number);
    }
    --depth;
    return result;
}

JsonParser::JsonParser(const std::string &text) : s(text) {}

Json JsonParser::parse() {
    auto j = read();
    space();
    if (p != s.size())
        fail("trailing data");
    return j;
}
} // namespace rt
