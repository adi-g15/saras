#pragma once

#include "ast.hpp"
#include "lexer.hpp"
#include "tokens.hpp"
#include "utf8.hpp"
#include "util.hpp"
#include "visualise.hpp"
#include <cstdio>
#include <exception>
#include <iostream>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/raw_ostream.h>
#include <memory>
#include <ostream>
#include <rang.hpp>
#include <variant>

extern Token CurrentToken;
extern Ptr<llvm::LLVMContext> LContext;
extern Ptr<llvm::IRBuilder<>> LBuilder;
extern Ptr<llvm::Module> LModule;

// Top level parsing
static auto HandleFunctionDefinition(bool print_ir = true) {
    auto expr = parseFunctionExpr();
    if (expr) {
        // std::cout << "Successfully parsed a function body" << std::endl;

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

static auto HandleExtern(bool print_ir = true) {
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

static auto HandleTopLevelExpression(bool print_ir = true) {
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

// top ::= definition | external | expression | ';'
void run_interpreter(
    /*bool no_print = false, */ bool parser_mode /*non interactive*/ = false) {
    // Initialise
    // Open a new context and module.
    LContext = std::make_unique<llvm::LLVMContext>();
    LModule = std::make_unique<llvm::Module>("SARAS JIT", *LContext);

    // Create a new builder for the module.
    LBuilder = std::make_unique<llvm::IRBuilder<>>(*LContext);

    auto visiter_run = overload{
        [&parser_mode](TOK_EOF &t) {
            std::cout << "Acting for EOF" << std::endl;
            std::exit(0);
        },
        [&parser_mode](TOK_EXTERN &t) {
            visualise_ast(HandleExtern(!parser_mode).get());
            if (parser_mode) {
                std::cout << "Saved parsed AST for extern declaration"
                          << std::endl;
            }
        },
        [&parser_mode](TOK_FN &t) {
            visualise_ast(HandleFunctionDefinition(!parser_mode).get());
            if (parser_mode) {
                std::cout << "Saved parsed AST for function" << std::endl;
            }
        },
        [&parser_mode](TOK_KEYWORDS &t) {
            visualise_ast(HandleTopLevelExpression(!parser_mode).get());
            if (parser_mode) {
                std::cout << "Saved parsed AST for top-level expression"
                          << std::endl;
            }
        },
        [&parser_mode](TOK_OTHER &t) {
            if (t == ';') {
                CurrentToken = get_next_token();
                return;
            }
            visualise_ast(HandleTopLevelExpression(!parser_mode).get());
            if (parser_mode) {
                std::cout << "Saved parsed AST for top-level expression"
                          << std::endl;
            }
        },
        [&parser_mode](TOK_NUMBER &t) {
            visualise_ast(HandleTopLevelExpression(!parser_mode).get());
            if (parser_mode) {
                std::cout << "Saved parsed AST for top-level expression"
                          << std::endl;
            }
        },
        [&parser_mode](TOK_IDENTIFIER &t) {
            visualise_ast(HandleTopLevelExpression(!parser_mode).get());
            if (parser_mode) {
                std::cout << "Saved parsed AST for top-level expression"
                          << std::endl;
            }
        },
    };

    // if (!parser_mode)
    std::cout << rang::fg::yellow << "--saras--> " << rang::style::reset;

    CurrentToken = get_next_token();
    while (true) {
        try {
            std::visit(visiter_run, CurrentToken);

        } catch (std::string &e) {
            std::cerr << e << std::endl;
        } catch (std::exception &e) {
            std::cerr << e.what() << std::endl;
        }

        // if (!parser_mode)
        std::cout << rang::fg::yellow << "--saras--> " << rang::style::reset;
    }

    // Print all generated code
    LModule->print(llvm::errs(), nullptr);

    if (parser_mode) {
        std::cout << rang::style::italic << rang::fg::green
                  << "Please look for graph*.png, for outputted abstract "
                     "syntax trees !"
                  << rang::style::reset << std::endl;
    }
}
