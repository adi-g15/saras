/*
    SARAS
*/

#include "ast.hpp"
#include "lexer.hpp"
#include "tokens.hpp"
#include "visualise.hpp"
#include <exception>
#include <iostream>
#include <variant>

Token CurrentToken;

int main() {
    // _DEBUG_read_tokens();

    CurrentToken = get_next_token();
    try {
        auto ast = parseBinaryExpr();

        visualise_ast(ast.get());
    } catch (std::string &s) {
        std::cout << s << std::endl;
    } catch ( std::exception& e ) {
        std::cout << e.what() << std::endl;
    }

    return 0;
}
