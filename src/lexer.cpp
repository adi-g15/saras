#include "lexer.hpp"
#include <cctype>
#include <cstdio>
#include <iostream>
#include <string>

Token get_next_token() {
    static int LastChar = ' ';  // int, not char, just that negative values make sense then, and getchar also returns int

    /* Ignore all whitespaces (also true for first call to this function) */
    while (isspace(LastChar)) {
        LastChar = getchar();
    }

    /* [A-Z|a-z] */
    if (isalpha(LastChar)) {
        /* [A-Z|a-z][A-Z|a-z|0-9]+ */
        while (isalnum(LastChar)) {
            DataStr += LastChar;
            LastChar = getchar();
        }

        if (DataStr == "fn") {
            return TOK_FN;
        } else if (DataStr == "extern") {
            return TOK_EXTERN;
        }

        return TOK_IDENTIFIER;
    } else if (isdigit(LastChar)) {
        /* [0-9] */
        DataStr = LastChar;

        LastChar = getchar();
        while (isdigit(LastChar) || (LastChar == '.')) {
            DataStr += LastChar;
            LastChar = getchar();
        }

        NumVal = std::stod(DataStr);
        return TOK_NUMBER;
    } else if ( LastChar == '#' ) { // it is a single-line comment
        // std::getline(std::cin, DataStr);    // read the line

        while ( LastChar != EOF && LastChar != '\n' && LastChar != '\r' ) {
            LastChar = getchar();
        }

        // Check if LastChar is EOF
        if( LastChar != EOF ) {
            return get_next_token();
        } else {
            return TOK_EOF;
        }
    }

    // NOTE TO CALLER: Only use DataStr[0], in case of TOK_OTHER
    DataStr = LastChar;

    LastChar = getchar();   // next call should use a different value of LastChar
    return TOK_OTHER;
}
