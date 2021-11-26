#pragma once

#include "tokens.hpp"
#include "ast.hpp"
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>

extern Token CurrentToken;
extern Ptr<llvm::LLVMContext> LContext;
extern Ptr<llvm::IRBuilder<>> LBuilder;
extern Ptr<llvm::Module> LModule;

// Top level parsing
static auto HandleFunctionDefinition(bool print_ir = true);

static auto HandleExtern(bool print_ir = true);

static auto HandleTopLevelExpression(bool print_ir = true);

// top ::= definition | external | expression | ';'
void run_interpreter(
    /*bool no_print = false, */ bool parser_mode /*non interactive*/ = false);
