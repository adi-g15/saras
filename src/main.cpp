/*
    SARAS
*/

#include "ast.hpp"
#include "interpreter.hpp"
#include "lexer.hpp"
#include "rang.hpp"
#include "tokens.hpp"
#include "visualise.hpp"
#include <argparse/argparse.hpp>
#include <exception>
#include <iostream>
#include <variant>

Token CurrentToken;

using argparse::ArgumentParser;

int main(int argc, char *argv[]) {
    ArgumentParser program("SARAS");

    program.add_argument("-l", "--lexer")
        .help("Stop at Lexer, only print Tokens read")
        .default_value(false)
        .implicit_value(true);
    program.add_argument("-p", "--parser")
        .help("Stop at Parser stage, saves Abstract Syntax Tree in graph*.png "
              "files")
        .default_value(false)
        .implicit_value(true);
    program.add_argument("-ir")
        .help(
            "Stop at IR stage, prints LLVM Intermediate Representation for all "
            "expressions and functions")
        .default_value(false)
        .implicit_value(true);
    program.add_argument("--interactive")
        .help("DEFAULT, provides an interactive console to type")
        .default_value(true);

    try {
        program.parse_args(argc, argv);
    } catch (const std::runtime_error &err) {
        std::cerr << rang::fgB::blue << err.what() << rang::style::reset
                  << std::endl;
        std::cerr << program;
        std::exit(1);
    }

    if (program["--lexer"] == true) {
        dump_all_tokens();
        return 0;
    } else if (program["--parser"] == true) {
        run_interpreter(true);

        return 0;
    }

    try {
        run_interpreter();
    } catch (std::string &s) {
        std::cerr << rang::bg::red << rang::fg::red << s << rang::style::reset
                  << std::endl;
    } catch (std::exception &e) {
        std::cerr << rang::bg::red << rang::fg::red << e.what()
                  << rang::style::reset << std::endl;
    }

    return 0;
}
