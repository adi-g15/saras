#include "ast.hpp"
#include "compiler.hpp"
#include "interpreter.hpp"
#include "lexer.hpp"
#include "rang.hpp"
#include "tokens.hpp"
#include "visualise.hpp"
#include <argparse/argparse.hpp>
#include <exception>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <istream>
#include <variant>

Token CurrentToken;
extern Ptr<llvm::LLVMContext> LContext;
extern Ptr<llvm::IRBuilder<>> LBuilder;
extern Ptr<llvm::Module> LModule;

std::basic_istream<char> *input = &std::cin;    // source code file, or std::cin

using argparse::ArgumentParser;

int main(int argc, char *argv[]) {
    ArgumentParser program("SARAS");

    program.add_argument("-l", "--lexer")
        .help("Stop at Lexer, only print Tokens read")
        .default_value(false);
    program.add_argument("-p", "--parser")
        .help("Stop at Parser stage, saves Abstract Syntax Tree in graph*.png "
              "files")
        .default_value(false);
    program.add_argument("-ir")
        .help(
            "Stop at IR stage, prints LLVM Intermediate Representation for all "
            "expressions and functions")
        .default_value(false);
    program.add_argument("--no-print-ir")
        .help("Don't print IR in Interpreter mode (default mode)")
        .default_value(false);
    program.add_argument("-c", "--compile").help("Compile provided filename");

    try {
        program.parse_args(argc, argv);
    } catch (const std::runtime_error &err) {
        std::cerr << rang::fgB::blue << err.what() << rang::style::reset
                  << std::endl;
        std::cerr << program;
        std::exit(1);
    }

    // Initialise interpreter
    // Open a new context and module.
    LContext = std::make_unique<llvm::LLVMContext>();
    LModule = std::make_unique<llvm::Module>("SARAS Interpreter", *LContext);

    // Create a new builder for the module.
    LBuilder = std::make_unique<llvm::IRBuilder<>>(*LContext);

    if (program["--lexer"] == true) {
        dump_all_tokens();
        return 0;
    } else if (program["--parser"] == true) {
        run_interpreter({"parser-mode"});
        return 0;
    } else if (program.present("--compile")) {
        if (program.get<std::string>("--compile").empty()) {
            std::cerr
                << rang::style::bold << rang::fg::red
                << "Error: " << rang::style::reset
                << "Filename expected, but not passed, see \"saras --help\""
                << std::endl;

            return 1;
        }

        auto filename = program.get<std::string>("--compile");
        auto object_filename =
            std::filesystem::path(filename)
                .stem()
                .string(); // https://stackoverflow.com/a/38463871/12339402

        auto source_code = std::ifstream(filename);
        input = &source_code;
        run_interpreter({"no-print-ir", "no-print-prompt"});

        auto *target_machine = InitialisationCompiler();
        return CompileToObjectFile(object_filename, target_machine);
    }

    try {
        if (program["--no-print-ir"] == true) {
            run_interpreter({"no-print-ir"});
        } else {
            run_interpreter();
        }
    } catch (std::string &s) {
        std::cerr << rang::style::bold << rang::fg::red << s
                  << rang::style::reset << std::endl;
    } catch (std::exception &e) {
        std::cerr << rang::style::bold << rang::fg::red << e.what()
                  << rang::style::reset << std::endl;
    }

    return 0;
}
