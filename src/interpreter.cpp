#include "interpreter.hpp"
#include "ast.hpp"
#include "lexer.hpp"
#include "utf8.hpp"
#include "visualise.hpp"
#include <cstdio>
#include <exception>
#include <iostream>
#include <istream>
#include <llvm/Support/raw_ostream.h>
#include <rang.hpp>

extern Ptr<llvm::LLVMContext> LContext;
extern Ptr<llvm::IRBuilder<>> LBuilder;
extern Ptr<llvm::Module> LModule;

Ptr<FunctionAST> HandleFunctionDefinition(bool print_ir) {
    auto expr = parseFunctionExpr();
    if (expr) {
        // std::cout << "Successfully parsed a function body" << std::endl;

        // Pretty print LLVM IR
        if (auto *FnIR = expr->codegen()) {
            if (print_ir)
                FnIR->print(llvm::errs());
            fprintf(stderr, "\n");
            // FnIR->viewCFG();
        }

    } else {
        std::cerr << "Failed to parse... Skipping" << std::endl;
        CurrentToken = get_next_token();
    }
    return expr;
}

Ptr<FunctionPrototypeAST> HandleExtern(bool print_ir) {
    auto expr = parseExternPrototypeExpr();
    if (expr) {
        // std::cout << "Successfully parsed an extern prototype" << std::endl;

        // Pretty print LLVM IR
        if (auto *FnIR = expr->codegen()) {
            if (print_ir)
                FnIR->print(llvm::errs());
            fprintf(stderr, "\n");
        }

    } else {
        std::cerr << "Failed to parse... Skipping" << std::endl;
        CurrentToken = get_next_token();
    }
    return expr;
}

// Top level parsing
Ptr<FunctionAST> HandleTopLevelExpression(bool print_ir) {
    auto expr = parseTopLevelExpr();
    if (expr) {
        // std::cout << "Successfully parsed a top level expression" <<
        // std::endl;

        // Pretty print LLVM IR
        if (auto *FnIR = expr->codegen()) {
            if (print_ir)
                FnIR->print(llvm::errs());
            fprintf(stderr, "\n");

            // Remove the anonymous expression.
            FnIR->eraseFromParent();
        }
    } else {
        std::cerr << "Failed to parse... Skipping" << std::endl;
        CurrentToken = get_next_token();
    }
    return expr;
}

void run_interpreter(std::unordered_set<std::string> options) {
    bool parser_mode = options.find("parser-mode") != options.end();
    bool no_print_ir = options.find("no-print-ir") != options.end();
    bool no_print_prompt = options.find("no-print-prompt") != options.end();

    bool EofEncountered = false;
    auto visiter_run = overload{
        [&](TOK_EOF &t) {
            std::cout << "Acting for EOF" << std::endl;
            EofEncountered = true;
        },
        [&](TOK_EXTERN &t) {
            visualise_ast(HandleExtern(!parser_mode && !no_print_ir).get());
            if (parser_mode) {
                std::cout << "Saved parsed AST for extern declaration"
                          << std::endl;
            }
        },
        [&](TOK_FN &t) {
            visualise_ast(
                HandleFunctionDefinition(!parser_mode && !no_print_ir).get());
            if (parser_mode) {
                std::cout << "Saved parsed AST for function" << std::endl;
            }
        },
        [&](TOK_KEYWORDS &t) {
            visualise_ast(
                HandleTopLevelExpression(!parser_mode && !no_print_ir).get());
            if (parser_mode) {
                std::cout << "Saved parsed AST for top-level expression"
                          << std::endl;
            }
        },
        [&](TOK_OTHER &t) {
            if (t == ';') {
                CurrentToken = get_next_token();
                return;
            }
            visualise_ast(
                HandleTopLevelExpression(!parser_mode && !no_print_ir).get());
            if (parser_mode) {
                std::cout << "Saved parsed AST for top-level expression"
                          << std::endl;
            }
        },
        [&](TOK_NUMBER &t) {
            visualise_ast(
                HandleTopLevelExpression(!parser_mode && !no_print_ir).get());
            if (parser_mode) {
                std::cout << "Saved parsed AST for top-level expression"
                          << std::endl;
            }
        },
        [&](TOK_IDENTIFIER &t) {
            visualise_ast(
                HandleTopLevelExpression(!parser_mode && !no_print_ir).get());
            if (parser_mode) {
                std::cout << "Saved parsed AST for top-level expression"
                          << std::endl;
            }
        },
    };

    // if (!parser_mode)
    if (!no_print_prompt)
        std::cout << rang::fg::yellow << "--saras--> " << rang::style::reset;

    CurrentToken = get_next_token();
    while (!EofEncountered) {
        try {
            std::visit(visiter_run, CurrentToken);

        } catch (std::string &e) {
            std::cerr << e << std::endl;
        } catch (std::exception &e) {
            std::cerr << e.what() << std::endl;
        }

        if (!no_print_prompt)
            std::cout << rang::fg::yellow << "--saras--> "
                      << rang::style::reset;
    }

    if (!no_print_ir && !no_print_prompt) {
        std::cout << std::endl
                  << rang::style::italic << rang::fg::green
                  << "<-------------All generated IR-------------->"
                  << rang::style::reset << std::endl;
        // Print all generated code
        LModule->print(llvm::errs(), nullptr);
    }

    if (parser_mode && !no_print_prompt) {
        std::cout << rang::style::italic << rang::fg::green
                  << "Please look for graph*.png, for outputted abstract "
                     "syntax trees !"
                  << rang::style::reset << std::endl;
    }
}
