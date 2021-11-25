#pragma once

#include "lexer.hpp"
#include "tokens.hpp"
#include <stdexcept>
#include <string>
#include <variant>

extern Token CurrentToken;

template <unsigned int LINE> void debug_assert(bool b) {
#ifdef DEBUG
    if (!b) {
        auto visiter_tok_to_str =
            overload{[](const TOK_EOF &) { return "EOF"; },
                     [](const TOK_FN &) { return "FN"; },
                     [](const TOK_EXTERN &) { return "EXTERN"; },
                     [](const TOK_IDENTIFIER &) { return "IDENTIFIER"; },
                     [](const TOK_KEYWORDS &) { return "KEYWORD"; },
                     [](const TOK_NUMBER &) { return "NUMBER"; },
                     [](const TOK_OTHER &) { return "OTHER"; }};

        auto visiter_datastr =
            overload{[](const TOK_EOF &t) -> utf8::string { return ""; },
                     [](const TOK_FN &t) -> utf8::string { return ""; },
                     [](const TOK_EXTERN &t) -> utf8::string { return ""; },
                     [](const TOK_IDENTIFIER &t) { return t.identifier_str; },
                     [](const TOK_KEYWORDS &t) { return t.str; },
                     [](const TOK_NUMBER &t) { return std::to_string(t.val); },
                     [](const TOK_OTHER &t) { return utf8::to_string(t.c); }};

        CurrentToken = get_next_token();
        throw std::logic_error(
            "Assertion failed at Line:" + std::to_string(LINE) + " !" +
            "\nCurrentToken = " + std::visit(visiter_tok_to_str, CurrentToken) +
            " { " + std::visit(visiter_datastr, CurrentToken) + " }");
    }
#endif
}
