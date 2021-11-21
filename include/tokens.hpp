#pragma once

#include "utf8.hpp"
#include <variant>

// https://stackoverflow.com/a/64018031/12339402
/* Instead of enums, using Rust like enums (with std::variant, and visitors) */

struct TOK_EOF {};

struct TOK_FN {};
struct TOK_EXTERN {};

struct TOK_IDENTIFIER {
    utf8::string identifier_str;
};
struct TOK_KEYWORDS {
    utf8::string str;
};
struct TOK_NUMBER {
    double val;
};
struct TOK_OTHER {
    utf8::_char c;
};

using Token = std::variant<TOK_EOF, TOK_FN, TOK_EXTERN, TOK_IDENTIFIER,
                           TOK_KEYWORDS, TOK_NUMBER, TOK_OTHER>;

/* Allows easy comparisons with say for eg. ')' */
inline bool operator==(const Token& t, utf8::_char c) {
    return std::holds_alternative<TOK_OTHER>(t) && (std::get<TOK_OTHER>(t).c == c);
}

inline bool operator!=(const Token& t, utf8::_char c) {
    return !(t == c);
}
