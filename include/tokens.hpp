#pragma once

#include <string>
#include <variant>

// https://arne-mertz.de/2018/05/overload-build-a-variant-visitor-on-the-fly/
// https://stackoverflow.com/a/64018031/12339402
/* Instead of enums, using Rust like enums (with std::variant, and visitors) */

struct TOK_EOF {};

struct TOK_FN {};
struct TOK_EXTERN {};

struct TOK_IDENTIFIER {
    std::string identifier_str;
};
struct TOK_KEYWORDS {
    std::string str;
};
struct TOK_NUMBER {
    double val;
};
struct TOK_OTHER { /*std::variant<int32_t, >*/
    std::string data;
};

using Token = std::variant<TOK_EOF, TOK_FN, TOK_EXTERN, TOK_IDENTIFIER,
                           TOK_KEYWORDS, TOK_NUMBER, TOK_OTHER>;

// Common pattern to allow overloading lambdas for use in std::visit
template <class... Ts> struct overload : Ts... { using Ts::operator()...; };
template <class... Ts> overload(Ts...) -> overload<Ts...>;
