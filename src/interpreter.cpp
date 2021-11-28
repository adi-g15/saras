#include "interpreter.hpp"
#include "ast.hpp"
#include "lexer.hpp"
#include "utf8.hpp"
#include "util.hpp"
#include "visualise.hpp"
#include <cstdio>
#include <exception>
#include <iostream>
#include <istream>
#include <llvm/ExecutionEngine/Orc/Core.h>
#include <llvm/ExecutionEngine/Orc/ExecutorProcessControl.h>
#include <llvm/ExecutionEngine/Orc/JITTargetMachineBuilder.h>
#include <llvm/ExecutionEngine/Orc/MachOPlatform.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <memory>
#include <rang.hpp>
#include <stdexcept>
#include <string>
#include <utility>

#include "JITClass.hpp"
extern Ptr<llvm::LLVMContext> LContext;
extern Ptr<llvm::IRBuilder<>> LBuilder;
extern Ptr<llvm::Module> LModule;

static Ptr<JITClass> saras_jit;
static llvm::ExitOnError ExitOnErr;

// static Ptr<llvm::legacy::PassManager> LFuncPassMngr;

void InitializeJIT() {
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmParser();
    llvm::InitializeNativeTargetAsmPrinter();

    auto execProcessControl = llvm::orc::SelfExecutorProcessControl::Create();
    if (!execProcessControl) {
        throw execProcessControl.takeError();
    }

    auto execSession = std::make_unique<llvm::orc::ExecutionSession>(
        std::move(*execProcessControl));

    auto JTMB = llvm::orc::JITTargetMachineBuilder(
        execSession->getExecutorProcessControl().getTargetTriple());

    auto dataLayout = JTMB.getDefaultDataLayoutForTarget();
    if (!dataLayout)
        throw dataLayout.takeError();

    saras_jit = std::make_unique<JITClass>(
        std::move(execSession), std::move(JTMB), std::move(*dataLayout));
}

void InitialisizeModuleContextJIT(bool compiler_mode) {
    // Initialise interpreter
    // Open a new context and module.
    if (!compiler_mode) {
        InitializeJIT();
    }

    LContext = std::make_unique<llvm::LLVMContext>();
    LModule = std::make_unique<llvm::Module>("SARAS Interpreter", *LContext);

    // Create a new builder for the module.
    LBuilder = std::make_unique<llvm::IRBuilder<>>(*LContext);

    if (!compiler_mode) {
        LModule->setDataLayout(saras_jit->getDataLayout());
    }
}

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
Ptr<FunctionAST> HandleTopLevelExpression(bool print_ir,
                                          bool compiler_mode = false) {
    auto expr = parseTopLevelExpr();
    if (expr) {
        // Pretty print LLVM IR
        if (auto *FnIR = expr->codegen()) {
            // if (print_ir)
            //     FnIR->print(llvm::errs());
            fprintf(stderr, "\n");

            if (!compiler_mode) {
                // Create a ResourceTracker to track JIT'd memory allocated to
                // the anonymous expression -- that way we can free it after
                // executing.
                auto RT = saras_jit->getMainJITDylib().createResourceTracker();

                auto TSM = llvm::orc::ThreadSafeModule(std::move(LModule),
                                                       std::move(LContext));
                ExitOnErr (saras_jit->addModule(std::move(TSM), RT));
                InitialisizeModuleContextJIT(compiler_mode);

                // Search the JIT for the __anon_expr symbol.
                auto ExprSymbol = ExitOnErr(saras_jit->lookup("__anon_expr"));

                // Get the symbol's address and cast it to the right type (takes
                // no arguments, returns a double) so we can call it as a native
                // function.
                double (*FP)() =
                    (double (*)())(intptr_t)ExprSymbol.getAddress();
                fprintf(stderr, "Evaluated to %f\n", FP());

                // Delete the anonymous expression module from the JIT.
                ExitOnErr(RT->remove());
            } else {
                // Remove the anonymous expression.
                FnIR->eraseFromParent();
            }
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
    bool compiler_mode = options.find("compilation-mode") != options.end();

    InitialisizeModuleContextJIT(compiler_mode);

    bool EofEncountered = false;
    auto visiter_run = overload{
        [&](TOK_EOF &t) {
            if (parser_mode) {
                std::cout << "Acting for EOF" << std::endl;
            }
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
            visualise_ast(HandleTopLevelExpression(!parser_mode && !no_print_ir,
                                                   compiler_mode)
                              .get());
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
            visualise_ast(HandleTopLevelExpression(!parser_mode && !no_print_ir,
                                                   compiler_mode)
                              .get());
            if (parser_mode) {
                std::cout << "Saved parsed AST for top-level expression"
                          << std::endl;
            }
        },
        [&](TOK_NUMBER &t) {
            visualise_ast(HandleTopLevelExpression(!parser_mode && !no_print_ir,
                                                   compiler_mode)
                              .get());
            if (parser_mode) {
                std::cout << "Saved parsed AST for top-level expression"
                          << std::endl;
            }
        },
        [&](TOK_IDENTIFIER &t) {
            visualise_ast(HandleTopLevelExpression(!parser_mode && !no_print_ir,
                                                   compiler_mode)
                              .get());
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
