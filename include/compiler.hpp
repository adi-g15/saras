#pragma once

#include <llvm/Target/TargetMachine.h>
#include <string_view>

llvm::TargetMachine *InitialisationCompiler();
int CompileToObjectFile(std::string_view filename,
                               llvm::TargetMachine *target_machine);
