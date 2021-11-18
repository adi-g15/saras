#pragma once

#include <cctype>
#include <cstdio>
#include <cstring>
#include <string>

static std::string DataStr;
static double NumVal;

// Else, other tokens
enum Token : int {
    TOK_EOF = -1,

    // Function & extern
    TOK_FN = -2,
    TOK_EXTERN = -3,

    // Identifiers (includes reserved tokens) & Numbers
    TOK_IDENTIFIER = -4,
    TOK_NUMBER = -5,

    TOK_OTHER
};

// The actual implementation of the lexer is a single function named gettok.
// ( aka gettok ) - return next token
Token get_next_token();
