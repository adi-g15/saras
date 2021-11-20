#pragma once

#include <cctype>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "tokens.hpp"

// clang-format off
// FUTURE: Use std::u8string instead
static const std::vector<std::string> LANG_KEYWORDS
    = {
        "if", "else", "for", "while", "return",
        "यदि", "अथवा", "लूप", "लघुलूप", "वापसी",
      };
// clang-format on

// The actual implementation of the lexer is a single function named gettok.
// ( aka gettok ) - return next token
Token get_next_token();

#ifdef DEBUG
void _DEBUG_read_tokens();
#endif
