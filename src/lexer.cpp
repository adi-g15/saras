#include "lexer.hpp"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <ranges>
#include <string>

#ifdef DEBUG
#include <iostream>
#include <tabulate/table.hpp>
#endif

Token get_next_token() {
    static int LastChar = ' '; // int, not char, just that negative values make
                               // sense then, and getchar also returns int

    /* Ignore all whitespaces (also true for first call to this function) */
    while (isspace(LastChar)) {
        LastChar = getchar();
    }

    /* [A-Z|a-z] */
    if (isalpha(LastChar)) {
        DataStr.clear();
        /* [A-Z|a-z][A-Z|a-z|0-9]+ */
        while (isalnum(LastChar)) {
            DataStr += LastChar;
            LastChar = getchar();
        }

        if (DataStr == "fn") {
            return TOK_FN;
        } else if (DataStr == "extern") {
            return TOK_EXTERN;
        } else if (std::ranges::find(LANG_KEYWORDS, DataStr) !=
                   LANG_KEYWORDS.cend()) {
            return TOK_KEYWORDS;
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
    } else if (LastChar == '#') { // it is a single-line comment
        // std::getline(std::cin, DataStr);    // read the line

        while (LastChar != EOF && LastChar != '\n' && LastChar != '\r') {
            LastChar = getchar();
        }

        // Case if it's EOF or not is handled by next call
        return get_next_token();
    } else if (LastChar == EOF) {
        return TOK_EOF;
    }

    // NOTE TO CALLER: Only use DataStr[0], in case of TOK_OTHER
    DataStr = LastChar;

    LastChar = getchar(); // next call should use a different value of LastChar
    return TOK_OTHER;
}

#ifdef DEBUG
void _DEBUG_read_tokens() {
    Token t = TOK_OTHER;

    using namespace tabulate;

    Table table;

    auto token_name = [&t]() -> const char * {
        switch (t) {
        case TOK_EOF:
            return "EOF";
        case TOK_FN:
            return "FN";
        case TOK_EXTERN:
            return "EXTERN";
        case TOK_IDENTIFIER:
            return "IDENTIFIER";
        case TOK_KEYWORDS:
            return "KEYWORD";
        case TOK_NUMBER:
            return "NUMBER";
        case TOK_OTHER:
            return "OTHER";
        default:
            return "----";
        }
    };

    table.add_row({"Token", "  DataStr  "});
    while (t != TOK_EOF) {
        t = get_next_token();

        table.add_row({token_name(), DataStr});
    }

    table[0]
        .format()
        .padding_top(1)
        .padding_bottom(1)
        .font_align(FontAlign::center)
        .font_style({FontStyle::underline})
        .font_background_color(Color::red);

    table.column(1).format().font_color(Color::yellow);
    table[0][1]
        .format()
        .font_background_color(Color::blue)
        .font_color(Color::white);
    std::cout << table << std::endl;
}
#endif
