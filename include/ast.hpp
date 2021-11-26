#pragma once

#include "tokens.hpp"
#include "utf8.hpp"
#include "util.hpp"
#include <llvm/IR/Function.h>
#include <llvm/IR/Value.h>
#include <map>
#include <memory>
#include <vector>

using std::vector;

// Base Class
struct ExprAST {
  public:
    virtual llvm::Value *codegen() = 0;
    virtual ~ExprAST() {}
};

// Number
struct NumberAST : public ExprAST {
    double value;

    llvm::Value *codegen() override;
    explicit NumberAST(double val) : value(val) {}
};

// Variable
struct VariableAST : public ExprAST {
    const utf8::string var_name;

    llvm::Value *codegen() override;
    explicit VariableAST(const utf8::string &var_name) : var_name(var_name) {}
};

static const std::map<utf8::_char, int> OPERATOR_PRECENDENCE_TABLE = {
    {'<', 5}, {'>', 5}, {'+', 10}, {'-', 10}, {'*', 20}, {'/', 20}};

// Binary Expressions
struct BinaryExprAST : public ExprAST {
    const utf8::_char opr;
    Ptr<ExprAST> lhs, rhs;

    virtual llvm::Value *codegen();
    BinaryExprAST(Ptr<ExprAST> lhs, const utf8::_char &opr, Ptr<ExprAST> rhs)
        : lhs(std::move(lhs)), opr(opr), rhs(std::move(rhs)) {}
};

// Expression class for if/then/else
struct IfExprAST : public ExprAST {
    Ptr<ExprAST> condition, then_, else_;

    llvm::Value *codegen();
    IfExprAST(Ptr<ExprAST> condition, Ptr<ExprAST> then_, Ptr<ExprAST> else_)
        : condition(std::move(condition)), then_(std::move(then_)),
          else_(std::move(else_)) {}
};

struct BlockAST : public ExprAST {
    const vector<Ptr<ExprAST>> expressions;

    llvm::Value *codegen();
    virtual llvm::Value *codegen(llvm::Function *func, bool is_if_else = false);

    BlockAST(vector<Ptr<ExprAST>> expressions)
        : expressions(std::move(expressions)) {}
};

// Function call
struct FunctionCallAST : public ExprAST {
    const utf8::string callee;
    const vector<Ptr<ExprAST>> args;

    virtual llvm::Value *codegen();
    FunctionCallAST(const utf8::string &callee, vector<Ptr<ExprAST>> args)
        : callee(callee), args(std::move(args)) {}
};

// Function prototype
struct FunctionPrototypeAST : public ExprAST {
    const vector<utf8::string> parameter_names;

    const utf8::string function_name;

    llvm::Function *codegen() override;
    FunctionPrototypeAST(const utf8::string &name,
                         const vector<utf8::string> &param_names)
        : function_name(name), parameter_names(param_names) {}
};

// Function
struct FunctionAST : public ExprAST {
    const Ptr<FunctionPrototypeAST> prototype;
    const Ptr<BlockAST> block;

    llvm::Function *codegen() override;
    FunctionAST(Ptr<FunctionPrototypeAST> prototype, Ptr<BlockAST> block)
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
Ptr<ExprAST> LogError(const utf8::string &str);
Ptr<FunctionPrototypeAST> LogErrorP(const utf8::string &str);
llvm::Value *LogErrorV(const utf8::string &str);

// These WON'T do error checking, if current token is okay

// 'Primary' expressions
Ptr<NumberAST> parseNumberExpr();
Ptr<ExprAST> parseParenExpr();
Ptr<ExprAST> parseIdentifierAndCalls();

Ptr<ExprAST> parsePrimaryExpression();
Ptr<ExprAST> parseBinaryHelperFn(Ptr<ExprAST> lhs, int min_precedence);

Ptr<ExprAST> parseIfExpr();

Ptr<FunctionPrototypeAST> parsePrototypeExpr();
Ptr<FunctionAST> parseFunctionExpr();
Ptr<BlockAST> parseBlock();
Ptr<ExprAST> parseExpression();
Ptr<FunctionPrototypeAST> parseExternPrototypeExpr();
Ptr<FunctionAST> parseTopLevelExpr();
