#pragma once

#include "util.hpp"
#include <array>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <string>
#include <variant>

using std::holds_alternative, std::monostate, std::array;

namespace utf8 {
/**
 * Problem using normal array types in std::variant:
 * Array types are non copy constructible (surprised but that seems to be the
 * case): std::is_copy_constructible_v<char[2]> is false
 *
 * And, hence the variant itself becomes non-copy_constructible
 *
 * So, using std::array instead*/
using _char =
    std::variant<char, array<uint8_t, 2>, array<uint8_t, 3>, array<uint8_t, 4>,
                 monostate>; /* If it is empty, that signifies EOF */

// FUTURE: Use std::u8string instead
using string = std::string;

// Considering ASCII to also include 'EOF' (which isn't the case, but helps
// logic)
inline bool is_not_ascii(const _char &c) {
    return !(holds_alternative<char>(c) || holds_alternative<monostate>(c));
}

inline bool isspace(const _char &c) {
    return (holds_alternative<char>(c) && (::isspace(std::get<char>(c))));
}

inline bool isalpha(const _char &c) {
    return (holds_alternative<char>(c) && (::isalpha(std::get<char>(c))));
}

inline bool isalnum(const _char &c) {
    return (holds_alternative<char>(c) && (::isalnum(std::get<char>(c))));
}

// check if digit
inline bool isdigit(const _char &c) {
    return (holds_alternative<char>(c) && (::isdigit(std::get<char>(c))));
}

inline bool is_eof(const _char &c) { return holds_alternative<monostate>(c); }

// Reference: There are some implicit properties of the bytes that can let us
// know if it is ascii or more digits of some utf-8 character follow
// https://en.wikipedia.org/wiki/UTF-8#Encoding
// read utf-8 character from stdin
// static, else it WILL cause multiple definitions linker error (even with
// #pragma once)
static _char get_character() {
    int tmp = getchar();

    if (tmp == EOF) {
        return monostate{};
    }

    uint8_t t = (uint8_t)tmp;

    if ((t & (1 << 7)) == 0) { /* See table on wikipedia, for 1 byte utf-8
                                  (ascii), the 8th bit is unset */
        return (char)t;
    } else if ((t & (1 << 5)) == 0) {
        /* 2 byte character */

        array<uint8_t, 2> c;
        c[0] = t;
        c[1] = getchar();

        return c;

    } else if ((t & (1 << 4)) == 0) {
        /* 3 byte character */

        array<uint8_t, 3> c;
        c[0] = t;
        c[1] = getchar();
        c[2] = getchar();

        return c;

    } else {
        /* 4 byte character */

        array<uint8_t, 4> c;
        c[0] = t;
        c[1] = getchar();
        c[2] = getchar();
        c[3] = getchar();

        return c;
    }
}

static std::string to_string(utf8::_char c);
} // namespace utf8

static inline bool operator==(const utf8::_char &uc, char c) {
    return holds_alternative<char>(uc) && (std::get<char>(uc) == c);
}

static inline bool operator!=(const utf8::_char &uc, char c) {
    return !(uc == c);
}

// Overloaded '+=' operator, so that UTF-8 character can be pushed to string
static void operator+=(utf8::string &s, const utf8::_char &c) {
    auto visiter = overload{
        [&s](char c) { s += c; },
        [&s](const array<uint8_t, 2> &c) {
            s += c[0];
            s += c[1];
        },
        [&s](const array<uint8_t, 3> &c) {
            s += c[0];
            s += c[1];
            s += c[2];
        },
        [&s](const array<uint8_t, 4> &c) {
            s += c[0];
            s += c[1];
            s += c[2];
            s += c[3];
        },
        [&s](monostate) { /*Ignore, it is EOF*/ },
    };

    std::visit(visiter, c);
}

static std::string utf8::to_string(utf8::_char c) {
    std::string s;
    s += c;
    return s;
}
