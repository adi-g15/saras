#pragma once

#include <cctype>
#include <cstdio>
#include <cstring>
#include <vector>

#include "tokens.hpp"
#include "utf8.hpp"

// clang-format off
static const std::vector<utf8::string> LANG_KEYWORDS
    = {
        "if", "then", "else", "for", "while", "return",
        "यदि","तब", "अथवा", "लूप", "लघुलूप", "वापसी",
        "ఉంటే", "అప్పుడు", "లేకపోతే", "లూప్", "అయితే", "తిరిగి"
      };
// clang-format on

// The actual implementation of the lexer is a single function named gettok.
// ( aka gettok ) - return next token
Token get_next_token();

void dump_all_tokens();
