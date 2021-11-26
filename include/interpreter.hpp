#pragma once

#include "ast.hpp"
#include "tokens.hpp"
#include "util.hpp"
#include <iostream>
#include <istream>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <unordered_set>

extern Token CurrentToken;

Ptr<FunctionAST> HandleFunctionDefinition(bool print_ir = true);
Ptr<FunctionPrototypeAST> HandleExtern(bool print_ir = true);
Ptr<FunctionAST> HandleTopLevelExpression(bool print_ir = true);

void run_interpreter(std::unordered_set<std::string> options = {});
