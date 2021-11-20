#include "lexer.hpp"
#include "tokens.hpp"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <string>
#include <variant>

#ifdef DEBUG
#include <iostream>
#include <tabulate/table.hpp>
#endif

using std::holds_alternative;

Token get_next_token() {
    static int LastChar = ' '; // int, not char, just that negative values make
                               // sense then, and getchar also returns int
    std::string data_str;

    /* Ignore all whitespaces (also true for first call to this function) */
    while (isspace(LastChar)) {
        LastChar = getchar();
    }

    /* [A-Z|a-z] */
    if (isalpha(LastChar)) {
        /* [A-Z|a-z][A-Z|a-z|0-9]+ */
        while (isalnum(LastChar)) {
            data_str += LastChar;
            LastChar = getchar();
        }

        if (data_str == "chakra") {
            return TOK_FN{};
        } else if (data_str == "extern") {
            return TOK_EXTERN{};
        } else if (std::find(LANG_KEYWORDS.cbegin(), LANG_KEYWORDS.cend(),
                             data_str) != LANG_KEYWORDS.cend()) {
            return TOK_KEYWORDS{data_str};
        }

        return TOK_IDENTIFIER{data_str};
    } else if (isdigit(LastChar)) {
        /* [0-9] */
        data_str = LastChar;

        LastChar = getchar();
        while (isdigit(LastChar) || (LastChar == '.')) {
            data_str += LastChar;
            LastChar = getchar();
        }

        return TOK_NUMBER{std::stod(data_str)};
    } else if (LastChar == '#') { // it is a single-line comment
        // std::getline(std::cin, DataStr);    // read the line

        while (LastChar != EOF && LastChar != '\n' && LastChar != '\r') {
            LastChar = getchar();
        }

        // Case if it's EOF or not is handled by next call
        return get_next_token();
    } else if (LastChar == EOF) {
        return TOK_EOF{};
    }

    // NOTE TO CALLER: Only use DataStr[0], in case of TOK_OTHER
    data_str = LastChar;

    LastChar = getchar(); // next call should use a different value of LastChar
    return TOK_OTHER{data_str};
}

#ifdef DEBUG
void _DEBUG_read_tokens() {
    Token t = TOK_OTHER{""};

    using namespace tabulate;

    Table table;

    auto visiter_tok_to_str =
        overload{[](const TOK_EOF &) { return "EOF"; },
                 [](const TOK_FN &) { return "FN"; },
                 [](const TOK_EXTERN &) { return "EXTERN"; },
                 [](const TOK_IDENTIFIER &) { return "IDENTIFIER"; },
                 [](const TOK_KEYWORDS &) { return "KEYWORD"; },
                 [](const TOK_NUMBER &) { return "NUMBER"; },
                 [](const TOK_OTHER &) { return "OTHER"; }};

    auto visiter_datastr =
        overload{[](const TOK_EOF &t) -> std::string { return ""; },
                 [](const TOK_FN &t) -> std::string { return ""; },
                 [](const TOK_EXTERN &t) -> std::string { return ""; },
                 [](const TOK_IDENTIFIER &t) { return t.identifier_str; },
                 [](const TOK_KEYWORDS &t) { return t.str; },
                 [](const TOK_NUMBER &t) { return std::to_string(t.val); },
                 [](const TOK_OTHER &t) { return t.data; }};

    auto token_name = std::visit(visiter_tok_to_str, t);
    auto token_datastr = std::visit(visiter_datastr, t);
    table.add_row({"Token", "  DataStr  "});
    while (holds_alternative<TOK_EOF>(t)) {
        t = get_next_token();

        table.add_row({token_name, token_datastr});
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
