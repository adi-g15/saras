#include "ast.hpp"
#include "lexer.hpp"
#include "tokens.hpp"
#include <iostream>
#include <memory>
#include <variant>

// Expects to be called when the current token is TOK_NUMBER
Ptr<ExprAST> parseExpression() {}

Ptr<NumberAST> parseNumberExpr() {
    if (!holds_alternative<TOK_NUMBER>(CurrentToken)) {
        // Current token is not a number

        return nullptr;
    }

    auto expr =
        std::make_unique<NumberAST>(std::get<TOK_NUMBER>(CurrentToken).val);

    CurrentToken = get_next_token();

    return expr;
}
Ptr<ExprAST> parseParenExpr();
Ptr<ExprAST> parseBinaryExpr();
Ptr<FunctionPrototypeAST> parsePrototypeExpr();
Ptr<FunctionAST> parseFunctionExpr();

/**
 * Interesting aspects of the LLVM's approach (not 'eating the last token'
 * here):
 *
 * The routines eat all tokens that correspond to the production and returns the
 * lexer buffer with the next token (OUR DOESN'T do this), which isn't part of
 * the grammar production, ready to go
 * Fairly a standard way to implement recursive descent parsers
 *
 * Look Ahead: having the next token (by calling get_next_token), but still
 * processing/working on the current token, then that next token is the
 * lookahead. For eg. currently read "factorial", then store this token/name in
 * a variable/string, then call get_next_token(), we get either '=' or '(', that
 * is the lookahead, now we can decide better, how to parse it, for eg. in first
 * case it is normal variable name, in second case it is 'likely' a function
 * call
 */

// paren = '(' expression ')'
Ptr<ExprAST> parseParenExpr() {
    if (!holds_alternative<TOK_OTHER>(CurrentToken)) {
        // Current token can't be TOK_OTHER, and then the data should be ')'

        return nullptr;
    }

    get_next_token(); // ignore the current token, ie. '('

    auto expression = parseExpression();
    if (!expression) {
        return nullptr;
    } else if (std::get<TOK_OTHER>(CurrentToken).data != ")") {
        return LogError("Expected ')'");
    } else {
        CurrentToken =
            get_next_token(); // set current token for next function calls

        return expression;
    }
}

Ptr<ExprAST> LogError(const std::string &str) {
    std::cerr << "LogError: " << str << '\n';
    return nullptr;
}

Ptr<FunctionPrototypeAST> LogErrorP(const std::string &str) {
    LogError(str);
    return nullptr;
}
