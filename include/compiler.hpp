#pragma once

#include <string>

#include <llvm/Target/TargetMachine.h>

llvm::TargetMachine *InitialisationCompiler();
int CompileToObjectFile(const std::string &filename,
                        llvm::TargetMachine *target_machine);
