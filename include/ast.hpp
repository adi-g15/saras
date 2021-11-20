#pragma once

#include "tokens.hpp"
#include <memory>
#include <string>
#include <vector>

static Token CurrentToken;

// Base Class
class ExprAST {
  public:
    virtual ~ExprAST() {}
};

template <typename T> using Ptr = std::unique_ptr<T>;

// Number
class NumberAST : ExprAST {
    double value;

  public:
    explicit NumberAST(double val) : value(val) {}
};

// Variable
class VariableAST : ExprAST {
    const std::string var_name;

  public:
    explicit VariableAST(const std::string &var_name) : var_name(var_name) {}
};

// Binary Expressions
class BinaryExprAsT : ExprAST {
    const std::string opr;
    Ptr<ExprAST> lhs, rhs;

  public:
    BinaryExprAsT(const std::string &opr, Ptr<ExprAST> lhs, Ptr<ExprAST> rhs)
        : opr(opr), lhs(std::move(lhs)), rhs(std::move(rhs)) {}
};

// Function call
class FunctionCallAST : ExprAST {
    const std::string callee;
    const std::vector<Ptr<ExprAST>> args;

  public:
    FunctionCallAST(const std::string &callee,
                    std::vector<Ptr<ExprAST>> args)
        : callee(callee), args(std::move(args)) {}
};

// Function prototype
class FunctionPrototypeAST : ExprAST {
    const std::string function_name;
    const std::vector<std::string> parameter_names;

  public:
    FunctionPrototypeAST(const std::string &name,
                         const std::vector<std::string> &param_names)
        : function_name(name), parameter_names(param_names) {}
};

// Function
class FunctionAST : ExprAST {
    const Ptr<FunctionPrototypeAST> prototype;
    const Ptr<ExprAST> block;

  public:
    FunctionAST(Ptr<FunctionPrototypeAST> prototype, Ptr<ExprAST> block)
        : prototype(std::move(prototype)), block(std::move(block)) {}
};

/**
 * functions are typed with just a count of their arguments. Since all values
 * are double precision floating point, the type of each argument doesn’t need
 * to be stored anywhere. In a more aggressive and realistic language, the
 * “ExprAST” class would probably have a type field
 */

// NOT using the CurToken & getNextToken as given in the tutorial

// Helper functions
Ptr<ExprAST> LogError(const std::string& str);
Ptr<FunctionPrototypeAST> LogErrorP(const std::string& str);

Ptr<ExprAST> parseExpression();
Ptr<NumberAST> parseNumberExpr();
Ptr<ExprAST> parseParenExpr();
Ptr<ExprAST> parseBinaryExpr();
Ptr<FunctionPrototypeAST> parsePrototypeExpr();
Ptr<FunctionAST> parseFunctionExpr();
