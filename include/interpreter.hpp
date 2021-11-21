#pragma once

#include "ast.hpp"
#include "lexer.hpp"
#include "tokens.hpp"
#include "utf8.hpp"
#include "util.hpp"
#include "visualise.hpp"
#include <iostream>
#include <ostream>
#include <variant>

extern Token CurrentToken;

// Top level parsing
static auto HandleFunctionDefinition() {
    auto expr = parseFunctionExpr();
    if (expr)
        std::cout << "Successfully parsed a function body" << std::endl;
    else {
        std::cerr << "Failed to parse... Skipping" << std::endl;
        CurrentToken = get_next_token();
    }
    return expr;
}

static auto HandleExtern() {
    auto expr = parseExternPrototypeExpr();
    if (expr)
        std::cout << "Successfully parsed an extern prototype" << std::endl;
    else {
        std::cerr << "Failed to parse... Skipping" << std::endl;
        CurrentToken = get_next_token();
    }
    return expr;
}

static auto HandleTopLevelExpression() {
    auto expr = parseTopLevelExpr();
    if (expr)
        std::cout << "Successfully parsed a top level expression" << std::endl;
    else {
        std::cerr << "Failed to parse... Skipping" << std::endl;
        CurrentToken = get_next_token();
    }
    return expr;
}

// top ::= definition | external | expression | ';'
void run_interpreter() {
    int num_lines = 0;
    auto visiter_run = overload{
        [&num_lines](TOK_EOF &t) {
            std::cout << "Acting for EOF" << std::endl;
            std::exit(0);
        },
        [&num_lines](TOK_EXTERN &t) {
            std::cout << "Acting for TOK_EXTERN" << std::endl;
            visualise_ast(HandleExtern().get());
        },
        [&num_lines](TOK_FN &t) {
            std::cout << "Acting for TOK_FN" << std::endl;
            visualise_ast(HandleFunctionDefinition().get());
        },
        [&num_lines](TOK_KEYWORDS &t) {
            std::cout << "Acting for TOK_KEYWORDS" << std::endl;
            visualise_ast(parseExpression().get());
        },
        [&num_lines](TOK_OTHER &t) {
            std::cout << "Acting for TOK_OTHER" << std::endl;
            if (t == ';')
                return;
            visualise_ast(parseExpression().get());
        },
        [&num_lines](TOK_NUMBER &t) {
            std::cout << "Acting for TOK_NUMBER" << std::endl;
            visualise_ast(parseNumberExpr().get());
        },
        [&num_lines](TOK_IDENTIFIER &t) {
            std::cout << "Acting for TOK_IDENTIFIER" << std::endl;
            visualise_ast(parseExpression().get());
        },
    };

    std::cout << "saras > ";
    std::cout.flush();
    CurrentToken = get_next_token();
    while (true) {
        std::visit(visiter_run, CurrentToken);

        std::cout << "saras > ";
        std::cout.flush();
    }
}
