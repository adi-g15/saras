#include "ast.hpp"
#include "assert.hpp"
#include "lexer.hpp"
#include "tokens.hpp"
#include "util.hpp"
#include <iostream>
#include <memory>
#include <variant>
#include <vector>

using std::holds_alternative;

/** @expects: CurrentToken is TOK_NUMBER
 *
 * @matches:
 * numexpr
 *   => any constant number
 */
Ptr<NumberAST> parseNumberExpr() {
    debug_assert(holds_alternative<TOK_NUMBER>(CurrentToken));

    auto expr =
        std::make_unique<NumberAST>(std::get<TOK_NUMBER>(CurrentToken).val);

    CurrentToken = get_next_token();

    return expr;
}

/** @expects: CurrentToken is TOK_IDENTIFIER
 *
 * @matches:
 * idexpr
 *   => identifier
 *   => func( exp1, exp2,... )
 */
Ptr<ExprAST> parseIdentifierAndCalls() {
    debug_assert(holds_alternative<TOK_IDENTIFIER>(CurrentToken));

    auto identifier = CurrentToken;

    // MUST update CurrentToken, since if it's not '(', then next function calls
    // expect the next tokens, in the other case (=='('), it will basically be
    // eaten/forgotten
    CurrentToken = get_next_token(); /* Since while working on identifier, we
                                        asked for next token, AND ALSO
                                        using/comparing it, that is lookahead*/
    if (CurrentToken /*lookahead*/ != '(')
        return std::make_unique<VariableAST>(
            std::get<TOK_IDENTIFIER>(CurrentToken).identifier_str);

    CurrentToken = get_next_token(); // eats '(' (eat means to 'forget'
                                     // about the last token)
    std::vector<Ptr<ExprAST>> args;

    while (CurrentToken != ')') {
        args.push_back(parseExpression());

        // the above call will have moved the lexer to next token, so
        // CurrentToken is likely ','
        if (CurrentToken == ')')
            break;

        if (CurrentToken != ',') {
            return LogError("Expected ',' in argument list.\nProbably you "
                            "typed something like: \"func(a b'\" and "
                            "forgot the ',' between a and b ?");
        } else {
            // eat ','
            CurrentToken = get_next_token();
        }
    }

    CurrentToken = get_next_token(); // eat ')', ie. forget it

    return std::make_unique<FunctionCallAST>(
        std::get<TOK_IDENTIFIER>(identifier).identifier_str, std::move(args));
}

/**
 * @expects: CurrentToken is '('
 *
 * @matches:
 * parenexp
 *   => ( expr )
 */
Ptr<ExprAST> parseParenExpr() {
    debug_assert(CurrentToken == '(');

    CurrentToken = get_next_token();

    auto expr = parseExpression();

    // Right now, CurrentToken should be at ')'
    if (CurrentToken != ')') {
        return LogError(
            "Expected a matching ')'.\nProbably you wrote something like "
            "\"(x+(y+2)\" and forgot a matching closing parenthesis");
    }

    CurrentToken = get_next_token(); // 'eat' the ')'
    return expr;
}

/**
 * @expects: CurrentToken is TOK_IDENTIFIER, TOK_NUMBER or '('
 */
Ptr<ExprAST> parsePrimaryExpression() {
    debug_assert(CurrentToken == '(' ||
                 holds_alternative<TOK_IDENTIFIER>(CurrentToken) ||
                 holds_alternative<TOK_NUMBER>(CurrentToken));

    if( CurrentToken == '(' ) {
        return parseParenExpr();
    } else if( holds_alternative<TOK_NUMBER>(CurrentToken) ) {
        return parseNumberExpr();
    } else if( holds_alternative<TOK_IDENTIFIER>(CurrentToken) ) {
        return parseIdentifierAndCalls();
    } else {
        return LogError("Wrong token passed that can't be handled by parsePrimaryExpression()");
    }
}

// Expects to be called when the current token is TOK_NUMBER
Ptr<ExprAST> parseExpression() {}

Ptr<ExprAST> parseBinaryExpr();
Ptr<FunctionPrototypeAST> parsePrototypeExpr();
Ptr<FunctionAST> parseFunctionExpr();

/**
 * Interesting aspects of the LLVM's approach (not 'eating the last token'
 * here):
 *
 * The routines eat all tokens that correspond to the production and returns
 * the lexer buffer with the next token (OUR DOESN'T do this), which isn't
 * part of the grammar production, ready to go Fairly a standard way to
 * implement recursive descent parsers
 *
 * Look Ahead: having the next token (by calling get_next_token), but still
 * processing/working on the current token, then that next token is the
 * lookahead. For eg. currently read "factorial", then store this token/name
 * in a variable/string, then call get_next_token(), we get either '=' or
 * '(', that is the lookahead, now we can decide better, how to parse it,
 * for eg. in first case it is normal variable name, in second case it is
 * 'likely' a function call
 */

Ptr<ExprAST> LogError(const utf8::string &str) {
    std::cerr << "LogError: " << str << '\n';
    return nullptr;
}

Ptr<FunctionPrototypeAST> LogErrorP(const utf8::string &str) {
    LogError(str);
    return nullptr;
}
