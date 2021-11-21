#pragma once

#include "ast.hpp"
#include "lexer.hpp"
#include "tokens.hpp"
#include "utf8.hpp"
#include "util.hpp"
#include <iostream>
#include <ostream>
#include <variant>

extern Token CurrentToken;

// Top level parsing
static void HandleFunctionDefinition() {
    auto expr = parseFunctionExpr();
    if (expr)
        std::cout << "Successfully parsed a function body" << std::endl;
    else {
        std::cerr << "Failed to parse... Skipping" << std::endl;
        CurrentToken = get_next_token();
    }
}

static void HandleExtern() {
    auto expr = parseExternPrototypeExpr();
    if (expr)
        std::cout << "Successfully parsed an extern prototype" << std::endl;
    else {
        std::cerr << "Failed to parse... Skipping" << std::endl;
        CurrentToken = get_next_token();
    }
}

static void HandleTopLevelExpression() {
    auto expr = parseTopLevelExpr();
    if (expr)
        std::cout << "Successfully parsed a top level expression" << std::endl;
    else {
        std::cerr << "Failed to parse... Skipping" << std::endl;
        CurrentToken = get_next_token();
    }
}

// top ::= definition | external | expression | ';'
void run_interpreter() {
    auto visiter = overload{
        [](TOK_EOF &t) { std::exit(0); },
        [](TOK_EXTERN &t) { HandleExtern(); },
        [](TOK_FN &t) { HandleFunctionDefinition(); },
        [](TOK_KEYWORDS &t) { parseExpression(); },
        [](TOK_OTHER &t) {
            parseExpression();
        },
        [](TOK_NUMBER &t) { parseNumberExpr(); },
        [](TOK_IDENTIFIER &t) {
            parseExpression();
        },
    };

    std::cout << "saras > ";
    CurrentToken = get_next_token();
    while (true) {
        std::cout << "saras > ";

        std::visit(visiter, CurrentToken);
    }
}
