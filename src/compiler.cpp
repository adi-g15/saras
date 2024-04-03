#include "compiler.hpp"
#include "util.hpp"
#include <exception>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/CodeGen.h>
#include <llvm/Support/Host.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <rang.hpp>

extern Ptr<llvm::Module> LModule;

llvm::TargetMachine *InitialisationCompiler() {
    auto TargetTriple = llvm::sys::getDefaultTargetTriple();

    // Initialise all targets for emitting code
    llvm::InitializeAllTargetInfos();
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmParsers();
    llvm::InitializeAllAsmPrinters();

    auto err_str = std::string();
    auto Target = llvm::TargetRegistry::lookupTarget(TargetTriple, err_str);

    if (Target == nullptr) {
        std::cerr << rang::style::bold << rang::fg::red
                  << "Failed to get Target for target triple=" << TargetTriple
                  << ": " << err_str << rang::style::reset;
        throw std::exception();
    }

    auto CPU = "generic"; // without any additional features, options or
                          // relocation model
    auto Features = "";   // no additional features
    llvm::TargetOptions opt;
    auto RM = std::optional<llvm::Reloc::Model>();

    auto TargetMachine =
        Target->createTargetMachine(TargetTriple, CPU, Features, opt, RM);

    // configure module, to specify the target and data layout
    LModule->setDataLayout(TargetMachine->createDataLayout());
    LModule->setTargetTriple(TargetTriple);

    return TargetMachine;
}

int CompileToObjectFile(const std::string &filename,
                        llvm::TargetMachine *target_machine) {
    std::error_code err_code;

    llvm::raw_fd_ostream destination(filename, err_code);

    if (err_code) {
        std::cerr << rang::style::bold << rang::fg::red
                  << "Could not open file: " << rang::style::reset
                  << err_code.message() << std::endl;
        return 1;
    }

    llvm::legacy::PassManager pass_mngr;
    const auto FILETYPE = llvm::CGFT_ObjectFile;

    // This method should return true if emission of this file type is not
    // supported, or false on success.
    if (target_machine->addPassesToEmitFile(pass_mngr, destination, nullptr,
                                            FILETYPE) == true) {
        throw std::logic_error("Can't emit file of given filetype !");
    }

    pass_mngr.run(*LModule);
    destination.flush();

    return 0;
}
