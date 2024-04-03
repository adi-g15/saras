#include "ast.hpp"
#include "assert.hpp"
#include "rang.hpp"
#include "tokens.hpp"
#include "utf8.hpp"
#include "util.hpp"

#include <algorithm>
#include <iostream>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

#include <llvm/ADT/APFloat.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/InstrTypes.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Verifier.h>

using llvm::BasicBlock;
using std::holds_alternative, std::make_unique;

extern Token CurrentToken;

Ptr<llvm::LLVMContext> LContext;
Ptr<llvm::IRBuilder<>> LBuilder;
Ptr<llvm::Module> LModule;

// NamedValues keeps tracks of variables defined in the current scope, and maps
// to their llvm representation
static std::map<utf8::string, llvm::Value *> NamedValues;

/**
 * Interesting aspects of the LLVM's approach (not 'eating the last token'
 * here):
 *
 * The routines eat all tokens that correspond to the production and returns
 * the lexer buffer with the next token (OUR DOESN'T do this), which isn't
 * part of the grammar production, ready to go Fairly a standard way to
 * implement recursive descent parsers
 *
 * Look Ahead: having the next token (by calling get_next_token), but still
 * processing/working on the current token, then that next token is the
 * lookahead. For eg. currently read "factorial", then store this token/name
 * in a variable/string, then call get_next_token(), we get either '=' or
 * '(', that is the lookahead, now we can decide better, how to parse it,
 * for eg. in first case it is normal variable name, in second case it is
 * 'likely' a function call
 */

/** @expects: CurrentToken is TOK_NUMBER
 *
 * @matches:
 * numexpr
 *   => any constant number
 */
Ptr<NumberAST> parseNumberExpr() {
    debug_assert<__LINE__>(holds_alternative<TOK_NUMBER>(CurrentToken));

    auto expr = make_unique<NumberAST>(std::get<TOK_NUMBER>(CurrentToken).val);

    CurrentToken = get_next_token();

    return expr;
}

/** @expects: CurrentToken is TOK_IDENTIFIER
 *
 * @matches:
 * idexpr
 *   => identifier
 *   => func( exp1, exp2,... )
 */
Ptr<ExprAST> parseIdentifierAndCalls() {
    debug_assert<__LINE__>(holds_alternative<TOK_IDENTIFIER>(CurrentToken));

    auto identifier = CurrentToken;

    // MUST update CurrentToken, since if it's not '(', then next function calls
    // expect the next tokens, in the other case (=='('), it will basically be
    // eaten/forgotten
    CurrentToken = get_next_token(); /* Since while working on identifier, we
                                        asked for next token, AND ALSO
                                        using/comparing it, that is lookahead*/
    if (CurrentToken /*lookahead*/ != '(')
        return make_unique<VariableAST>(
            std::get<TOK_IDENTIFIER>(identifier).identifier_str);

    CurrentToken = get_next_token(); // eats '(' (eat means to 'forget'
                                     // about the last token)
    std::vector<Ptr<ExprAST>> args;

    while (CurrentToken != ')') {
        args.push_back(parseExpression());

        // the above call will have moved the lexer to next token, so
        // CurrentToken is likely ','
        if (CurrentToken == ')')
            break;

        if (CurrentToken != ',') {
            return LogError("Expected ',' in argument list\n\t\tProbably you "
                            "typed something like: \"func(a b\" and "
                            "forgot the ',' between a and b ?");
        } else {
            // eat ','
            CurrentToken = get_next_token();
        }
    }

    CurrentToken = get_next_token(); // eat ')', ie. forget it

    return make_unique<FunctionCallAST>(
        std::get<TOK_IDENTIFIER>(identifier).identifier_str, std::move(args));
}

/**
 * @expects: CurrentToken is '('
 *
 * @matches:
 * parenexp
 *   => ( expr )
 */
Ptr<ExprAST> parseParenExpr() {
    debug_assert<__LINE__>(CurrentToken == '(');

    CurrentToken = get_next_token();

    auto expr = parseExpression();

    // Right now, CurrentToken should be at ')'
    if (CurrentToken != ')') {
        return LogError(
            "Expected a matching ')'\n\t\tProbably you wrote something like "
            "\"(x+(y+2)\" and forgot a matching closing parenthesis");
    }

    CurrentToken = get_next_token(); // 'eat' the ')'
    return expr;
}

/**
 * @expects: CurrentToken is TOK_IDENTIFIER, TOK_KEYWORDS, TOK_NUMBER or '('
 */
Ptr<ExprAST> parsePrimaryExpression() {
    debug_assert<__LINE__>(CurrentToken == '(' ||
                           holds_alternative<TOK_IDENTIFIER>(CurrentToken) ||
                           holds_alternative<TOK_NUMBER>(CurrentToken) ||
                           holds_alternative<TOK_KEYWORDS>(CurrentToken));

    if (CurrentToken == ';')
        return nullptr; // ignore ';'

    if (CurrentToken == '(') {
        return parseParenExpr();
    } else if (holds_alternative<TOK_NUMBER>(CurrentToken)) {
        return parseNumberExpr();
    } else if (holds_alternative<TOK_IDENTIFIER>(CurrentToken)) {
        return parseIdentifierAndCalls();
    } else if (holds_alternative<TOK_KEYWORDS>(CurrentToken)) {
        auto &keyword = std::get<TOK_KEYWORDS>(CurrentToken).str;

        if (keyword == "if" || keyword == "यदि" || keyword == "ఉంటే") {
            return parseIfExpr();
        }
    }
    return LogError("Wrong token passed that can't be handled by "
                    "parsePrimaryExpression()");
}

/**
 * @expects: CurrentToken is Primary Token(TOK_IDENTIFIER or TOK_NUMBER or '(')
 *           ie. an expression can be parsed
 *
 * @matches:
 * expr
 *   => expr
 *   => expr (binary_operator, expr)*
 **/
Ptr<ExprAST> parseExpression() {
    /* First try parsing a primary expression
     * Then, we can simply pass it as LHS to ParseBinaryHelperFn, since it will
     * simply return the LHS when the next token isn't found to be an operator*/
    return parseBinaryHelperFn(parsePrimaryExpression(), 0);
}

int GetPrecedence(utf8::_char c) {
    auto p = OPERATOR_PRECENDENCE_TABLE.find(c);
    if (p == OPERATOR_PRECENDENCE_TABLE.cend())
        return -1;
    return p->second;
}

/**
 * @expects: Called by parseExpression()
 *
 * @returns Returns computed expression as LHS once a token has precendence of
 * < min_precedence
 *
 * @note: Only 'a' is also acceptable (the 'binaryexpr => expr' case)
 **/
Ptr<ExprAST> parseBinaryHelperFn(Ptr<ExprAST> lhs, int min_precedence) {
    auto lookahead = CurrentToken; // should be operator

    // Operators exist ONLY when CurrentToken is TOK_OTHER
    // return LHS itself, if current token isn't an operator
    if (!holds_alternative<TOK_OTHER>(lookahead)) {
        // if CurrentToken isn't an operator, then return lhs
        // the 'binaryexpr => expr' case
        return lhs;
    }
    if (!lhs) {
        return lhs;
    }

    auto binary_opr = std::get<TOK_OTHER>(CurrentToken).c; // = lookahead
    while (GetPrecedence(binary_opr) >= min_precedence) {
        // At the end of this while loop, we do a parseBinaryHelperFn, which
        // advances to next token, which maybe EOF etc., so break (this
        // condition may have also been added to the above while loop)
        if (!holds_alternative<TOK_OTHER>(CurrentToken))
            break;

        binary_opr = std::get<TOK_OTHER>(CurrentToken).c; // = lookahead
        auto opr_precedence = GetPrecedence(binary_opr);

        CurrentToken = get_next_token(); // eat binary operator
        auto rhs = parsePrimaryExpression();

        // parsePrimary reads the next token, so CurrentToken is updated
        lookahead = CurrentToken;

        // while (GetPrecedence(std::get<TOK_OTHER>(lookahead).c) >=
        //        opr_precedence) {
        //     rhs = parseBinaryHelperFn(std::move(rhs), opr_precedence + 1);
        //     lookahead; parsePrimary reads the next token, so CurrentToken is
        //     updated
        // }
        /* Why the additional check ? Because it may be ';', or EOF */
        if (holds_alternative<TOK_OTHER>(lookahead) &&
            GetPrecedence(std::get<TOK_OTHER>(lookahead).c) > opr_precedence) {
            rhs = parseBinaryHelperFn(std::move(rhs), opr_precedence + 1);
            // parsePrimary reads the next token, so CurrentToken is updated
            lookahead = CurrentToken; // lookahead
        }

        if (!rhs)
            return nullptr;

        lhs = make_unique<BinaryExprAST>(std::move(lhs), binary_opr,
                                         std::move(rhs));
        if (holds_alternative<TOK_OTHER>(lookahead)) {
            // modify binary_opr to current token's character value, else it
            // will become an infinite loop
            binary_opr = std::get<TOK_OTHER>(lookahead).c;
        } else {
            binary_opr = '\0';
        }
    }

    return lhs;
}

/**
 * @expects: CurrentToken == "if"
 **/
Ptr<ExprAST> parseIfExpr() {
    debug_assert<__LINE__>(holds_alternative<TOK_KEYWORDS>(CurrentToken));

    auto is_tok_if = []() {
        return holds_alternative<TOK_KEYWORDS>(CurrentToken) &&
               (std::get<TOK_KEYWORDS>(CurrentToken).str == "if" ||
                std::get<TOK_KEYWORDS>(CurrentToken).str == "यदि" ||
                std::get<TOK_KEYWORDS>(CurrentToken).str == "ఉంటే");
    };
    auto is_tok_then = []() {
        return holds_alternative<TOK_KEYWORDS>(CurrentToken) &&
               (std::get<TOK_KEYWORDS>(CurrentToken).str == "then" ||
                std::get<TOK_KEYWORDS>(CurrentToken).str == "तब" ||
                std::get<TOK_KEYWORDS>(CurrentToken).str == "అప్పుడు");
    };
    auto is_tok_else = []() {
        return holds_alternative<TOK_KEYWORDS>(CurrentToken) &&
               (std::get<TOK_KEYWORDS>(CurrentToken).str == "else" ||
                std::get<TOK_KEYWORDS>(CurrentToken).str == "अथवा" ||
                std::get<TOK_KEYWORDS>(CurrentToken).str == "లేకపోతే");
    };

    if (!is_tok_if()) {
        return LogError(
            "Expected \"if\" (or equivalent keyword in hindi/telugu) expression");
    }

    CurrentToken = get_next_token(); // eat 'if' token

    auto condition = parseExpression(); // can also parse with or without
                                        // parenthesis (primary expr)

    if (!condition)
        return nullptr;

    if (!is_tok_then())
        return LogError("Expected \"then\" or equivalent keyword");

    CurrentToken = get_next_token(); // eat 'then'

    auto then_block = parseBlock();

    if (!is_tok_else())
        return LogError("Expected \"else\" or equivalent keyword");

    CurrentToken = get_next_token(); // eat 'else'

    auto else_block = parseBlock();

    if (!then_block || !else_block)
        return nullptr;

    return make_unique<IfExprAST>(std::move(condition), std::move(then_block),
                                  std::move(else_block));
}

/**
 * @expects: CurrentToken is TOK_IDENTIFIER (ie. name of function)
 *
 * @matches:
 *   expr
 *     => id '(' id, id, ... ')'
 **/
Ptr<FunctionPrototypeAST> parsePrototypeExpr() {
    debug_assert<__LINE__>(holds_alternative<TOK_IDENTIFIER>(CurrentToken));

    auto function_name = CurrentToken;

    if (!holds_alternative<TOK_IDENTIFIER>(CurrentToken)) {
        return LogErrorP("Expected function name in prototype");
    }

    CurrentToken = get_next_token();

    if (CurrentToken != '(') {
        return LogErrorP(
            "Expected '(' after function name in prototype\n\t\tProbably you "
            "forgot '(' after func in \"fn func\" ?");
    }

    CurrentToken = get_next_token(); // eat '('

    std::vector<utf8::string> arg_names;

    while (holds_alternative<TOK_IDENTIFIER>(CurrentToken)) {
        arg_names.push_back(
            std::get<TOK_IDENTIFIER>(CurrentToken).identifier_str);

        CurrentToken = get_next_token();
        if (CurrentToken == ')')
            break;

        if (CurrentToken != ',') {
            return LogErrorP(
                "Expected ',' or ')' in function arguments portion of the "
                "prototype\n\t\tProbably you forgot a comma in between two "
                "argument names, for eg, this will error: \"fn func(a b)\"");
        }

        CurrentToken = get_next_token(); // eat ','
    }

    CurrentToken = get_next_token(); // eat ')'
    return make_unique<FunctionPrototypeAST>(
        std::get<TOK_IDENTIFIER>(function_name).identifier_str, arg_names);
}

/**
 * @expects: CurrentToken == '{', or at start of an expression */
Ptr<BlockAST> parseBlock() {
    std::vector<Ptr<ExprAST>> expressions;
    if (CurrentToken == '{') {
        CurrentToken = get_next_token(); // eat '{'

        while (CurrentToken != '}') {
            // TODO: Decide whether to eat ';' in parseExpression()
            auto expr = parseExpression();

            if (!expr) {
                return nullptr;
            }
            expressions.push_back(std::move(expr));

            CurrentToken = get_next_token();
            if (holds_alternative<TOK_EOF>(CurrentToken)) {
                LogErrorP(
                    "Expected closing '}' for code block\n\t\tProbably you "
                    "missed a '}' corresponding to a previous '}");
                return nullptr;
            } else if (CurrentToken == ';') {
                CurrentToken = get_next_token(); // eat ';'
            }
        }

        CurrentToken = get_next_token(); // eat '}'
    } else {
        // Simply return the next expression
        auto expr = parseExpression();
        if (!expr)
            return nullptr;

        expressions.push_back(std::move(expr));
    }
    return make_unique<BlockAST>(std::move(expressions));
}

/**
 * @expects: CurrentToken is TOK_FN, ie. holding the function's name
 *
 * @matches:
 *   expr => 'fn' prototype expression
 *
 * @note - The expression field is the body, currently single expression
 */
Ptr<FunctionAST> parseFunctionExpr() {
    debug_assert<__LINE__>(holds_alternative<TOK_FN>(CurrentToken));

    CurrentToken = get_next_token(); // eat 'fn' keyword
    auto prototype = parsePrototypeExpr();
    auto body = parseBlock();

    if (!prototype || !body)
        return nullptr;

    return make_unique<FunctionAST>(std::move(prototype), std::move(body));
}

/**
 * @expects: CurrentToken is TOK_EXTERN
 *
 * @matches:
 *   expr => extern fn_prototype
 */
Ptr<FunctionPrototypeAST> parseExternPrototypeExpr() {
    debug_assert<__LINE__>(holds_alternative<TOK_EXTERN>(CurrentToken));

    CurrentToken = get_next_token(); // eat 'extern' keyword
    return parsePrototypeExpr();
}

/**
 * @expects: Any token that can be used/built into an expression
 *
 * @use: Helps compute top level expressions also, for eg. like in python/other
 * interpreters, type 'x+5', then output the result, same way, what we do is,
 * treat it like a function with that body, ie.
 *
 * fn ()
 *   x+5
 *
 * So, nothing to add, just create a nullary (zero arguments) function with the
 * expression as its body, so it can be evaluated as any other function
 *
 * @matches:
 * toplevelexpr => expression
 */
Ptr<FunctionAST> parseTopLevelExpr() {
    auto expr = parseBlock();
    if (!expr)
        return nullptr;

    return make_unique<FunctionAST>(
        make_unique<FunctionPrototypeAST>("", std::vector<utf8::string>()),
        std::move(expr));
}

// codegen implementations
llvm::Value *NumberAST::codegen() {
    // in the LLVM IR that constants are all uniqued together and shared. For
    // this reason, the API uses the “foo::get(…)” idiom instead of “new
    // foo(..)” or “foo::Create(..)”
    return llvm::ConstantFP::get(*LContext, llvm::APFloat(value));
}

llvm::Value *VariableAST::codegen() {
    auto V = NamedValues[var_name];
    if (!V)
        LogErrorV("Unknown variable: " + var_name);
    return V;
}

llvm::Value *BinaryExprAST::codegen() {
    llvm::Value *lhs_codegen = lhs->codegen();
    llvm::Value *rhs_codegen = rhs->codegen();

    if (!lhs_codegen || !rhs_codegen) {
        LogError(
            "Invalid LHS or RHS of binary expression\n\t\tMaybe you used an "
            "not-yet-defined variable ?");
        return nullptr;
    }

    if (opr == '+') {
        return LBuilder->CreateFAdd(lhs_codegen, rhs_codegen, "addtmp");
    } else if (opr == '-') {
        return LBuilder->CreateFSub(lhs_codegen, rhs_codegen, "subtmp");
    } else if (opr == '*') {
        return LBuilder->CreateFMul(lhs_codegen, rhs_codegen, "multmp");
    } else if (opr == '/') {
        return LBuilder->CreateFDiv(lhs_codegen, rhs_codegen, "divtmp");
    } else if (opr == '<') {
        // My way:
        auto L = LBuilder->CreateFCmpULT(lhs_codegen, rhs_codegen, "cmplttmp");
        // converting 0/1 (bool treated as int), to double
        return LBuilder->CreateUIToFP(L, llvm::Type::getDoubleTy(*LContext));
    } else if (opr == '>') {
        auto L = LBuilder->CreateFCmpUGT(lhs_codegen, rhs_codegen, "cmpgttmp");
        return LBuilder->CreateUIToFP(L, llvm::Type::getDoubleTy(*LContext));
    } else {
        throw std::logic_error(
            "An operator not handled: '" + utf8::to_string(opr) +
            "', IMPLEMENT IT. Line: " + std::to_string(__LINE__));
    }
}

llvm::Value *IfExprAST::codegen() {
    auto cond_ir = condition->codegen();
    if (!cond_ir)
        return nullptr;
    // COME HERE
    // "ONE" -> Ordered and not equal
    // Create a (condition != 0.0) instruction, ie. true for not zero, ie. true
    // for 1, ie. true for true ;D
    cond_ir = LBuilder->CreateFCmpONE(
        /*lhs*/ cond_ir,
        /*rcond_ir*/ llvm::ConstantFP::get(*LContext, llvm::APFloat(0.0)),
        "if_condn");

    // gets the current Function object that is being built. It gets this by
    // asking the builder for the current BasicBlock, and asking that block for
    // its “parent” (the function it is currently embedded into)
    auto parent_func = LBuilder->GetInsertBlock()->getParent();

    /** If the Parent parameter (3rd param) is specified, the basic block is
     *automatically inserted at either the end of the function (if InsertBefore
     *is 0), or before the specified basic block. */
    auto then_bb = BasicBlock::Create(*LContext, "then_bb", parent_func);
    auto else_bb = BasicBlock::Create(*LContext, "else_bb");
    auto cont_bb = BasicBlock::Create(*LContext, "continued_bb");

    // create 'conditional' br, ie. jump to then block if condition true, or
    // else block
    LBuilder->CreateCondBr(cond_ir, then_bb, else_bb);

    // add code to the end of then_bb, ie. add the return
    LBuilder->SetInsertPoint(then_bb);

    // Now actual add the IR for then and else blocks
    auto then_ir = then_->codegen();

    if (!then_ir)
        return nullptr;

    // creates uncondition 'br' label
    LBuilder->CreateBr(cont_bb);

    /**
     * Why then, are we getting the current block when we just set it to ThenBB
     * 5 lines above? The problem is that the “Then” expression may actually
     * itself change the block that the Builder is emitting into if, for
     * example, it contains a nested “if/then/else” expression. Because calling
     * codegen() recursively could arbitrarily change the notion of the current
     * block, we are required to get an up-to-date value for code that will set
     * up the Phi node.
     */
    then_bb =
        LBuilder->GetInsertBlock(); // codegen of 'Then' can change the current
                                    // block, update ThenBB for the PHI.

    // push else block to parent function
    parent_func->insert(parent_func->end(), else_bb);
    LBuilder->SetInsertPoint(else_bb);

    auto else_ir = else_->codegen();
    if (!else_ir)
        return nullptr;

    LBuilder->CreateBr(cont_bb);

    // codegen of 'Else' could have changed the current block, update ElseBB for
    // the PHI.
    else_bb = LBuilder->GetInsertBlock();
    parent_func->insert(parent_func->end(), cont_bb);
    LBuilder->SetInsertPoint(cont_bb);
    llvm::PHINode *phi_node =
        LBuilder->CreatePHI(llvm::Type::getDoubleTy(*LContext), 2, "cont_phi");

    phi_node->addIncoming(then_ir, then_bb);
    phi_node->addIncoming(else_ir, else_bb);

    return phi_node;
}

llvm::Value *BlockAST::codegen() {
    auto *block = LBuilder->GetInsertBlock();

    if (!block) {
        LogErrorP(
            "BlockAST::codegen requires a function, failed to autodetect");
        return nullptr;
    }

    auto *func = block->getParent();

    if (func != nullptr)
        return codegen(func, /*is_if_else*/ true);

    LogErrorP("BlockAST::codegen requires a function, failed to autodetect");
    return nullptr;
}

llvm::Value *BlockAST::codegen(llvm::Function *func, bool is_if_else) {
    // NOTE: @adi Temporary Solution, try to implement multi expression if
    // blocks
    if (!is_if_else) {
        // Create a basic block to start insertion into
        // > Basic blocks in LLVM are an important part of functions that define
        // the Control Flow Graph
        auto *block = BasicBlock::Create(*LContext, "entry", func);

        LBuilder->SetInsertPoint(block);

        if (expressions.size() > 1) {
            // tells the builder that new instructions should be inserted into
            // the end of the new basic block
            for (auto i = 0; i < (expressions.size() - 1); ++i) {
                expressions[i]->codegen();
            }
        }
    }

    // Returning return value, ie. of last expression
    return expressions.back()->codegen();
}

llvm::Value *FunctionCallAST::codegen() {
    // Look name in global module table
    llvm::Function *CalleeFunction = LModule->getFunction(callee);

    if (!CalleeFunction) {
        return LogErrorV("Unknown function referenced: " + callee);
    }

    // Verify number of arguments is same (Type is double always neverthless)
    if (CalleeFunction->arg_size() != args.size()) {
        return LogErrorV("Wrong number of arguments passed: Expected: " +
                         std::to_string(CalleeFunction->arg_size()) +
                         ", Actual Passed: " + std::to_string(args.size()));
    }

    std::vector<llvm::Value *> PassedArgs(args.size());
    std::transform(args.cbegin(), args.cend(), PassedArgs.begin(),
                   [](const auto &a) { return a->codegen(); });

    if (std::any_of(PassedArgs.cbegin(), PassedArgs.cend(),
                    [](const auto *e) { return e == nullptr; }))
        return nullptr;

    return LBuilder->CreateCall(CalleeFunction, PassedArgs, callee);
}

llvm::Function *FunctionPrototypeAST::codegen() {
    std::vector<llvm::Type *> ParameterTypes(parameter_names.size());

    // there are N doubles
    std::fill(ParameterTypes.begin(), ParameterTypes.end(),
              llvm::Type::getDoubleTy(*LContext));

    auto *func_type = llvm::FunctionType::get(
        llvm::Type::getDoubleTy(*LContext), ParameterTypes, false);
    auto *func =
        llvm::Function::Create(func_type, llvm::Function::ExternalLinkage,
                               function_name, LModule.get());

    unsigned idx = 0;
    for (auto &param : func->args()) {
        param.setName(parameter_names[idx++]);
    }

    return func;
}

llvm::Function *FunctionAST::codegen() {
    // Check, if the function name has already been declared (due to a previous
    // "extern")
    auto *func = LModule->getFunction(this->prototype->function_name);

    if (!func) {
        func = prototype->codegen();
    }

    if (!func) {
        return nullptr;
    }

    /**
     * @bug: This code does have a bug, though: If the FunctionAST::codegen()
     * method finds an existing IR Function, it does not validate its signature
     * against the definition’s own prototype. This means that an earlier
     * ‘extern’ declaration will take precedence over the function definition’s
     * signature, which can cause codegen to fail, for instance if the function
     * arguments are named differently.
     *  There are a number of ways to fix this bug, see what you can come up
     * with! Here is a testcase:

     *  extern foo(a);     # ok, defines foo.
     *  def foo(b) b;      # Error: Unknown variable name. (decl using 'a' takes
     *  precedence).

     **/

    // Check if function is NOT empty, ie. it has a function definition
    if (func->empty() == false) {
        LogErrorV("Cannot redefine function: " + prototype->function_name);
        return nullptr;
    }
    // add the function arguments to the NamedValues map (after first clearing
    // it out) so that they’re accessible to VariableExprAST nodes.
    NamedValues.clear();
    for (auto &param : func->args()) {
        NamedValues.insert_or_assign(param.getName().str(), &param);
    }

    auto *retval = this->block->codegen(func);
    if (retval) {
        LBuilder->CreateRet(retval);

        llvm::verifyFunction(*func);

        return func;
    } else {
        // Error reading body, remove function
        func->eraseFromParent();
        return nullptr;
    }
}

Ptr<ExprAST> LogError(const utf8::string &str) {
    std::cerr << rang::style::bold << rang::fg::red
              << "LogError: " << rang::style::reset << str << '\n';
    return nullptr;
}

Ptr<FunctionPrototypeAST> LogErrorP(const utf8::string &str) {
    LogError(str);
    return nullptr;
}

llvm::Value *LogErrorV(const utf8::string &err) {
    LogError(err);
    return nullptr;
}
