#pragma once

#include "tokens.hpp"
#include "utf8.hpp"
#include <map>
#include <memory>
#include <vector>

// Base Class
class ExprAST {
  public:
    virtual ~ExprAST() {}
};

template <typename T> using Ptr = std::unique_ptr<T>;

// Number
class NumberAST : public ExprAST {
    double value;

  public:
    explicit NumberAST(double val) : value(val) {}

    friend void recursive_ast(ExprAST *e, int &max_idx, std::ofstream &fout);
};

// Variable
class VariableAST : public ExprAST {
    const utf8::string var_name;

  public:
    explicit VariableAST(const utf8::string &var_name) : var_name(var_name) {}

    friend void recursive_ast(ExprAST *e, int &max_idx, std::ofstream &fout);
};

static const std::map<utf8::_char, int> OPERATOR_PRECENDENCE_TABLE = {
    {'<', 5},
    {'+', 10},
    {'-', 10},
    {'*', 20},
};

// Binary Expressions
class BinaryExprAST : public ExprAST {
    const utf8::_char opr;
    Ptr<ExprAST> lhs, rhs;

  public:
    BinaryExprAST(Ptr<ExprAST> lhs, const utf8::_char &opr, Ptr<ExprAST> rhs)
        : lhs(std::move(lhs)), opr(opr), rhs(std::move(rhs)) {}

    friend void recursive_ast(ExprAST *e, int &max_idx, std::ofstream &fout);
};

// Function call
class FunctionCallAST : public ExprAST {
    const utf8::string callee;
    const std::vector<Ptr<ExprAST>> args;

  public:
    FunctionCallAST(const utf8::string &callee, std::vector<Ptr<ExprAST>> args)
        : callee(callee), args(std::move(args)) {}

    friend void recursive_ast(ExprAST *e, int &max_idx, std::ofstream &fout);
};

// Function prototype
class FunctionPrototypeAST : public ExprAST {
    const utf8::string function_name;
    const std::vector<utf8::string> parameter_names;

  public:
    FunctionPrototypeAST(const utf8::string &name,
                         const std::vector<utf8::string> &param_names)
        : function_name(name), parameter_names(param_names) {}

    friend void recursive_ast(ExprAST *e, int &max_idx, std::ofstream &fout);
};

// Function
class FunctionAST : public ExprAST {
    const Ptr<FunctionPrototypeAST> prototype;
    const Ptr<ExprAST> block;

  public:
    FunctionAST(Ptr<FunctionPrototypeAST> prototype, Ptr<ExprAST> block)
        : prototype(std::move(prototype)), block(std::move(block)) {}

    friend void recursive_ast(ExprAST *e, int &max_idx, std::ofstream &fout);
};

/**
 * functions are typed with just a count of their arguments. Since all values
 * are double precision floating point, the type of each argument doesn’t need
 * to be stored anywhere. In a more aggressive and realistic language, the
 * “ExprAST” class would probably have a type field
 */

// NOT using the CurToken & getNextToken as given in the tutorial

// Helper functions
Ptr<ExprAST> LogError(const utf8::string &str);
Ptr<FunctionPrototypeAST> LogErrorP(const utf8::string &str);

// These WON'T do error checking, if current token is okay

// 'Primary' expressions
Ptr<NumberAST> parseNumberExpr();
Ptr<ExprAST> parseParenExpr();
Ptr<ExprAST> parseIdentifierAndCalls();

Ptr<ExprAST> parsePrimaryExpression();

Ptr<ExprAST> parseBinaryHelperFn(Ptr<ExprAST> lhs, int min_precedence);

Ptr<FunctionPrototypeAST> parsePrototypeExpr();
Ptr<FunctionAST> parseFunctionExpr();
Ptr<ExprAST> parseExpression();
Ptr<FunctionPrototypeAST> parseExternPrototypeExpr();
Ptr<FunctionAST> parseTopLevelExpr();
