#pragma once

#include <cctype>
#include <cstdio>
#include <cstring>
#include <string>

static std::string DataStr;
static double NumVal;

// Else, other tokens
enum Token : int {
    TOK_EOF,

    // Function & extern
    TOK_FN,
    TOK_EXTERN,

    // Identifiers (includes reserved tokens) & Numbers
    TOK_IDENTIFIER,
    TOK_NUMBER,

    TOK_OTHER
};

// The actual implementation of the lexer is a single function named gettok.
// ( aka gettok ) - return next token
Token get_next_token();

#ifdef DEBUG
void _DEBUG_read_tokens();
#endif
