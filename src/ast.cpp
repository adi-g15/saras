#include "ast.hpp"
#include "assert.hpp"
#include "lexer.hpp"
#include "tokens.hpp"
#include "utf8.hpp"
#include "util.hpp"
#include <iostream>
#include <memory>
#include <variant>
#include <vector>

using std::holds_alternative;

extern Token CurrentToken;

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

/** @expects: CurrentToken is TOK_NUMBER
 *
 * @matches:
 * numexpr
 *   => any constant number
 */
Ptr<NumberAST> parseNumberExpr() {
    debug_assert<__LINE__>(holds_alternative<TOK_NUMBER>(CurrentToken));

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
    debug_assert<__LINE__>(holds_alternative<TOK_IDENTIFIER>(CurrentToken));

    auto identifier = CurrentToken;

    // MUST update CurrentToken, since if it's not '(', then next function calls
    // expect the next tokens, in the other case (=='('), it will basically be
    // eaten/forgotten
    CurrentToken = get_next_token(); /* Since while working on identifier, we
                                        asked for next token, AND ALSO
                                        using/comparing it, that is lookahead*/
    if (CurrentToken /*lookahead*/ != '(')
        return std::make_unique<VariableAST>(
            std::get<TOK_IDENTIFIER>(identifier).identifier_str);

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
    debug_assert<__LINE__>(CurrentToken == '(');

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
    debug_assert<__LINE__>(CurrentToken == '(' ||
                           holds_alternative<TOK_IDENTIFIER>(CurrentToken) ||
                           holds_alternative<TOK_NUMBER>(CurrentToken));

    if( CurrentToken == ';' )   return nullptr;    // ignore ';'

    if (CurrentToken == '(') {
        return parseParenExpr();
    } else if (holds_alternative<TOK_NUMBER>(CurrentToken)) {
        return parseNumberExpr();
    } else if (holds_alternative<TOK_IDENTIFIER>(CurrentToken)) {
        return parseIdentifierAndCalls();
    } else {
        return LogError("Wrong token passed that can't be handled by "
                        "parsePrimaryExpression()");
    }
}

/**
 * @expects: CurrentToken is Primary Token(TOK_IDENTIFIER or TOK_NUMBER or '(')
 *           ie. an expression can be parsed
 *
 * @matches:
 * expr
 *   => expr
 *   => expr (binary_operator, expr)*
 **/
Ptr<ExprAST> parseExpression() {
    /* First try parsing a primary expression
     * Then, we can simply pass it as LHS to ParseBinaryHelperFn, since it will
     * simply return the LHS when the next token isn't found to be an operator*/
    return parseBinaryHelperFn(parsePrimaryExpression(), 0);
}

int GetPrecedence(utf8::_char c) {
    auto p = OPERATOR_PRECENDENCE_TABLE.find(c);
    if (p == OPERATOR_PRECENDENCE_TABLE.cend())
        return -1;
    return p->second;
}

/**
 * @expects: Called by parseExpression()
 *
 * @returns Returns computed expression as LHS once a token has precendence of
 * < min_precedence
 *
 * @note: Only 'a' is also acceptable (the 'binaryexpr => expr' case)
 **/
Ptr<ExprAST> parseBinaryHelperFn(Ptr<ExprAST> lhs, int min_precedence) {
    auto lookahead = CurrentToken; // should be operator

    // Operators exist ONLY when CurrentToken is TOK_OTHER
    // return LHS itself, if current token isn't an operator
    if (!holds_alternative<TOK_OTHER>(lookahead)) {
        // if CurrentToken isn't an operator, then return lhs
        // the 'binaryexpr => expr' case
        return lhs;
    }
    if (!lhs) {
        return lhs;
    }

    auto binary_opr = std::get<TOK_OTHER>(CurrentToken).c; // = lookahead
    while (GetPrecedence(binary_opr) >= min_precedence) {
        // At the end of this while loop, we do a parseBinaryHelperFn, which
        // advances to next token, which maybe EOF etc., so break (this
        // condition may have also been added to the above while loop)
        if (!holds_alternative<TOK_OTHER>(CurrentToken))
            break;

        binary_opr = std::get<TOK_OTHER>(CurrentToken).c; // = lookahead
        auto opr_precedence = GetPrecedence(binary_opr);

        CurrentToken = get_next_token(); // eat binary operator
        auto rhs = parsePrimaryExpression();

        // parsePrimary reads the next token, so CurrentToken is updated
        lookahead = CurrentToken;

        // while (GetPrecedence(std::get<TOK_OTHER>(lookahead).c) >=
        //        opr_precedence) {
        //     rhs = parseBinaryHelperFn(std::move(rhs), opr_precedence + 1);
        //     lookahead; parsePrimary reads the next token, so CurrentToken is
        //     updated
        // }
        /* Why the additional check ? Because it may be ';', or EOF */
        if (holds_alternative<TOK_OTHER>(lookahead) &&
            GetPrecedence(std::get<TOK_OTHER>(lookahead).c) > opr_precedence) {
            rhs = parseBinaryHelperFn(std::move(rhs), opr_precedence + 1);
            // parsePrimary reads the next token, so CurrentToken is updated
            lookahead = CurrentToken; // lookahead
        }

        if (!rhs)
            return nullptr;

        lhs = std::make_unique<BinaryExprAST>(std::move(lhs), binary_opr,
                                              std::move(rhs));
    }

    return lhs;
}

/**
 * @expects: CurrentToken is TOK_IDENTIFIER (ie. name of function)
 *
 * @matches:
 *   expr
 *     => id '(' id, id, ... ')'
 **/
Ptr<FunctionPrototypeAST> parsePrototypeExpr() {
    debug_assert<__LINE__>(holds_alternative<TOK_IDENTIFIER>(CurrentToken));

    auto function_name = CurrentToken;

    if (!holds_alternative<TOK_IDENTIFIER>(CurrentToken)) {
        return LogErrorP("Expected function name in prototype");
    }

    CurrentToken = get_next_token();

    if (CurrentToken != '(') {
        return LogErrorP(
            "Expected '(' after function name in prototype.\nProbably you "
            "forgot '(' after func in \"fn func\" ?");
    }

    CurrentToken = get_next_token(); // eat '('

    std::vector<utf8::string> arg_names;

    while (holds_alternative<TOK_IDENTIFIER>(CurrentToken)) {
        arg_names.push_back(
            std::get<TOK_IDENTIFIER>(CurrentToken).identifier_str);

        CurrentToken = get_next_token();
        if (CurrentToken == ')')
            break;

        if (CurrentToken != ',') {
            return LogErrorP(
                "Expected ',' or ')' in function arguments portion of the "
                "prototype.\nProbably you forgot a comma in between two "
                "argument names, for eg, this will error: \"fn func(a b)\"");
        }

        CurrentToken = get_next_token(); // eat ','
    }

    CurrentToken = get_next_token(); // eat ')'
    return std::make_unique<FunctionPrototypeAST>(
        std::get<TOK_IDENTIFIER>(function_name).identifier_str, arg_names);
}

/**
 * @expects: CurrentToken is TOK_FN, ie. holding the function's name
 *
 * @matches:
 *   expr => 'fn' prototype expression
 *
 * @note - The expression field is the body, currently single expression
 */
Ptr<FunctionAST> parseFunctionExpr() {
    debug_assert<__LINE__>(holds_alternative<TOK_FN>(CurrentToken));

    CurrentToken = get_next_token(); // eat 'fn' keyword
    auto prototype = parsePrototypeExpr();
    auto body = parseExpression();

    if (!prototype || !body)
        return nullptr;

    return std::make_unique<FunctionAST>(std::move(prototype), std::move(body));
}

/**
 * @expects: CurrentToken is TOK_EXTERN
 *
 * @matches:
 *   expr => extern fn_prototype
 */
Ptr<FunctionPrototypeAST> parseExternPrototypeExpr() {
    debug_assert<__LINE__>(holds_alternative<TOK_EXTERN>(CurrentToken));

    get_next_token(); // eat 'extern' keyword
    return parsePrototypeExpr();
}

/**
 * @expects: Any token that can be used/built into an expression
 *
 * @use: Helps compute top level expressions also, for eg. like in python/other
 * interpreters, type 'x+5', then output the result, same way, what we do is,
 * treat it like a function with that body, ie.
 *
 * fn ()
 *   x+5
 *
 * So, nothing to add, just create a nullary (zero arguments) function with the
 * expression as its body, so it can be evaluated as any other function
 *
 * @matches:
 * toplevelexpr => expression
 */
Ptr<FunctionAST> parseTopLevelExpr() {
    auto expr = parseExpression();
    if (!expr)
        return nullptr;

    return std::make_unique<FunctionAST>(
        std::make_unique<FunctionPrototypeAST>("", std::vector<utf8::string>()),
        std::move(expr));
}

Ptr<ExprAST> LogError(const utf8::string &str) {
    std::cerr << "LogError: " << str << '\n';
    return nullptr;
}

Ptr<FunctionPrototypeAST> LogErrorP(const utf8::string &str) {
    LogError(str);
    return nullptr;
}
