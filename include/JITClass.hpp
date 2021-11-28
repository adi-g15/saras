#pragma once

#include "util.hpp"
#include <llvm/ADT/StringRef.h>
#include <llvm/ExecutionEngine/Orc/CompileUtils.h>
#include <llvm/ExecutionEngine/Orc/Core.h>
#include <llvm/ExecutionEngine/Orc/ExecutionUtils.h>
#include <llvm/ExecutionEngine/Orc/IRCompileLayer.h>
#include <llvm/ExecutionEngine/Orc/JITTargetMachineBuilder.h>
#include <llvm/ExecutionEngine/Orc/Mangling.h>
#include <llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h>
#include <llvm/ExecutionEngine/SectionMemoryManager.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/Support/Error.h>
#include <memory>

class JITClass {
    Ptr<llvm::orc::ExecutionSession> exec_session;

    llvm::DataLayout data_layout;
    llvm::orc::MangleAndInterner mangle;

    llvm::orc::RTDyldObjectLinkingLayer object_link_layer;
    llvm::orc::IRCompileLayer compile_layer;

    llvm::orc::JITDylib &MainJD;

  public:
    JITClass(Ptr<llvm::orc::ExecutionSession> exec_session,
             llvm::orc::JITTargetMachineBuilder target_machine_builder,
             llvm::DataLayout data_layout)
        : exec_session(std::move(exec_session)), data_layout(data_layout),
          mangle(*(this->exec_session), data_layout),
          object_link_layer(
              *this->exec_session,
              []() { return std::make_unique<llvm::SectionMemoryManager>(); }),
          compile_layer(*this->exec_session, this->object_link_layer,
                        std::make_unique<llvm::orc::ConcurrentIRCompiler>(
                            std::move(target_machine_builder))),
          MainJD(this->exec_session->createBareJITDylib("<main>"))

    {
        MainJD.addGenerator(llvm::cantFail(
            llvm::orc::DynamicLibrarySearchGenerator::GetForCurrentProcess(
                data_layout.getGlobalPrefix())));

        // fixes "Resolving symbol with incorrect flags" errors on Windows
        if (target_machine_builder.getTargetTriple().isOSBinFormatCOFF()) {
            object_link_layer.setOverrideObjectFlagsWithResponsibilityFlags(
                true);
            object_link_layer.setAutoClaimResponsibilityForObjectSymbols(true);
        }
    }

    llvm::DataLayout getDataLayout() { return data_layout; }

    llvm::orc::JITDylib &getMainJITDylib() { return MainJD; }

    llvm::Error addModule(llvm::orc::ThreadSafeModule TSM,
                          llvm::orc::ResourceTrackerSP RT = nullptr) {
        if (!RT)
            RT = MainJD.getDefaultResourceTracker();
        return compile_layer.add(RT, std::move(TSM));
    }

    llvm::Expected<llvm::JITEvaluatedSymbol> lookup(llvm::StringRef Name) {
        return exec_session->lookup({&MainJD}, mangle(Name));
    }

    ~JITClass() {
        if (auto err = exec_session->endSession()) {
            exec_session->reportError(std::move(err));
        }
    }
};
