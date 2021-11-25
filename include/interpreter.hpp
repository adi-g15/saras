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
#include <variant>

extern Token CurrentToken;
extern Ptr<llvm::LLVMContext> LContext;
extern Ptr<llvm::IRBuilder<>> LBuilder;
extern Ptr<llvm::Module> LModule;

// Top level parsing
static auto HandleFunctionDefinition() {
    auto expr = parseFunctionExpr();
    if (expr) {
        std::cout << "Successfully parsed a function body" << std::endl;

        // Pretty print LLVM IR
        if (auto *FnIR = expr->codegen()) {
            // FnIR->print(llvm::errs());
            FnIR->print(llvm::errs());
            fprintf(stderr, "\n");
        }

    } else {
        std::cerr << "Failed to parse... Skipping" << std::endl;
        CurrentToken = get_next_token();
    }
    return expr;
}

static auto HandleExtern() {
    auto expr = parseExternPrototypeExpr();
    if (expr) {
        std::cout << "Successfully parsed an extern prototype" << std::endl;

        // Pretty print LLVM IR
        if (auto *FnIR = expr->codegen()) {
            FnIR->print(llvm::errs());
            fprintf(stderr, "\n");
        }

    } else {
        std::cerr << "Failed to parse... Skipping" << std::endl;
        CurrentToken = get_next_token();
    }
    return expr;
}

static auto HandleTopLevelExpression() {
    auto expr = parseTopLevelExpr();
    if (expr) {
        std::cout << "Successfully parsed a top level expression" << std::endl;

        // Pretty print LLVM IR
        if (auto *FnIR = expr->codegen()) {
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
void run_interpreter() {
    // Initialise
    // Open a new context and module.
    LContext = std::make_unique<llvm::LLVMContext>();
    LModule = std::make_unique<llvm::Module>("SARAS JIT", *LContext);

    // Create a new builder for the module.
    LBuilder = std::make_unique<llvm::IRBuilder<>>(*LContext);

    auto visiter_run = overload{
        [](TOK_EOF &t) {
            std::cout << "Acting for EOF" << std::endl;
            std::exit(0);
        },
        [](TOK_EXTERN &t) {
            std::cout << "Acting for TOK_EXTERN" << std::endl;
            visualise_ast(HandleExtern().get());
        },
        [](TOK_FN &t) {
            std::cout << "Acting for TOK_FN" << std::endl;
            visualise_ast(HandleFunctionDefinition().get());
        },
        [](TOK_KEYWORDS &t) {
            std::cout << "Handling top level expression" << std::endl;
            visualise_ast(HandleTopLevelExpression().get());
        },
        [](TOK_OTHER &t) {
            std::cout << "Acting for TOK_OTHER" << std::endl;
            if (t == ';') {
                CurrentToken = get_next_token();
                return;
            }
            std::cout << "Handling top level expression" << std::endl;
            visualise_ast(HandleTopLevelExpression().get());
        },
        [](TOK_NUMBER &t) {
            std::cout << "Handling top level expression" << std::endl;
            visualise_ast(HandleTopLevelExpression().get());
        },
        [](TOK_IDENTIFIER &t) {
            std::cout << "Handling top level expression" << std::endl;
            visualise_ast(HandleTopLevelExpression().get());
        },
    };

    std::printf("---------------------------------------- > ");
    // std::printf("saras > ");
    CurrentToken = get_next_token();
    while (true) {
        try {
            std::visit(visiter_run, CurrentToken);

        } catch (std::string &e) {
            std::cerr << e << std::endl;
        } catch (std::exception &e) {
            std::cerr << e.what() << std::endl;
        }

        std::printf("----------------------------------------- > ");
        // std::printf("saras > ");
    }

    // Print all generated code
    LModule->print(llvm::errs(), nullptr);
}
