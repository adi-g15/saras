#pragma once

#include <cctype>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

static std::string DataStr;
static double NumVal;

static const std::vector<std::string> LANG_KEYWORDS = {"if", "else", "for", "while", "return"};

// Else, other tokens
enum Token : int {
    TOK_EOF,

    // Function & extern
    TOK_FN,
    TOK_EXTERN,

    // Identifiers, Keywords & Numbers
    TOK_IDENTIFIER,
    TOK_KEYWORDS,
    TOK_NUMBER,

    TOK_OTHER
};

// The actual implementation of the lexer is a single function named gettok.
// ( aka gettok ) - return next token
Token get_next_token();

#ifdef DEBUG
void _DEBUG_read_tokens();
#endif
