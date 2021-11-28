#include "ast.hpp"
#include "compiler.hpp"
#include "interpreter.hpp"
#include "lexer.hpp"
#include "rang.hpp"
#include "tokens.hpp"
#include "visualise.hpp"
#include <cxxopts.hpp>
#include <exception>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <llvm/Support/Error.h>
#include <llvm/Support/raw_ostream.h>
#include <variant>

Token CurrentToken;
extern Ptr<llvm::LLVMContext> LContext;
extern Ptr<llvm::IRBuilder<>> LBuilder;
extern Ptr<llvm::Module> LModule;

std::basic_istream<char> *input = &std::cin; // source code file, or std::cin

int main(int argc, char *argv[]) {
    cxxopts::Options options("saras", "A compiler frontend");

    // clang-format off
    options.add_options()
        ("l,lexer", "Stop at Lexer, only print Tokens read")
        ("p,parse", "Stop at Parser stage, saves Abstract Syntax Tree in "
                     "graph*.png files")
        ("ir", "Stop at IR stage, prints LLVM Intermediate Representation for "
                "all expressions and functions")
        ("no-print-ir", "Don't print IR in Interpreter mode (default mode)")
        ("c,compile", "Compile provided filename", cxxopts::value<std::string>())
        ("h,help", "Print usage");
    // clang-format on

    options.allow_unrecognised_options();

    cxxopts::ParseResult result;
    try {
        result = options.parse(argc, argv);
    } catch (const std::runtime_error &err) {
        std::cerr << rang::fgB::blue << err.what() << rang::style::reset
                  << std::endl;
        std::cout << options.help();
        return 1;
    }

    if (result.count("help")) {
        std::cout << options.help() << std::endl;
        return 0;
    }

    if (result.count("lexer")) {
        dump_all_tokens();
        return 0;
    } else if (result.count("parser")) {
        run_interpreter({"parser-mode"});
        return 0;
    } else if (result.count("compile")) {
        auto filename = result["compile"].as<std::string>();
        if (filename.empty()) {
            std::cerr
                << rang::style::bold << rang::fg::red
                << "Error: " << rang::style::reset
                << "Filename expected, but not passed, see \"saras --help\""
                << std::endl;

            return 1;
        }

        // https://stackoverflow.com/a/38463871/12339402
        auto object_filename =
            std::filesystem::path(filename).stem().string() + ".o";

        auto source_code = std::ifstream(filename);
        input = &source_code;
        run_interpreter({"no-print-ir", "no-print-prompt", "compilation-mode"});

        auto *target_machine = InitialisationCompiler();
        return CompileToObjectFile(object_filename, target_machine);
    }

    try {
        if (result.count("--no-print-ir")) {
            run_interpreter({"no-print-ir"});
        } else {
            run_interpreter();
        }
    } catch (std::string &s) {
        std::cerr << rang::style::bold << rang::fg::red
                  << "ERROR: " << rang::style::reset << s << std::endl;
    } catch (std::exception &e) {
        std::cerr << rang::style::bold << rang::fg::red
                  << "ERROR: " << rang::style::reset << e.what() << std::endl;
    } catch(llvm::Error& e) {
        std::cerr << "ERROR: " << std::endl;
        llvm::errs() << e;
        // std::cerr << e << std::endl;
    }

    return 0;
}
