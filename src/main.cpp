/*
    SARAS
*/

#include "lexer.hpp"
#include "tokens.hpp"
#include <iostream>
#include <variant>

int main() {
    _DEBUG_read_tokens();

    // utf8::_char c;

    // auto printer = overload{
    //     [](char c) { std::cout << c << std::endl; },
    //     [](const array<uint8_t, 2> &c) {
    //         std::cout << c[0] << c[1] << std::endl;
    //     },
    //     [](const array<uint8_t, 3> &c) {
    //         std::cout << c[0] << c[1] << c[2] << std::endl;
    //     },
    //     [](const array<uint8_t, 4> &c) {
    //         std::cout << c[0] << c[1] << c[2] << c[3] << std::endl;
    //     },
    //     [](monostate) { std::cout << "EOF" << std::endl; },
    // };

    // do {
    //     std::visit(printer, c);
    //     c = get_character();

    //     std::cout << c.index() << ": ";
    // } while (!is_eof(c));
}
