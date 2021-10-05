/*
 * Copyright © 2019 Paweł Dziepak
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include <filesystem>
#include <unordered_map>

#include <llvm/ExecutionEngine/JITEventListener.h>

#include <llvm/ExecutionEngine/Orc/CompileUtils.h>
#include <llvm/ExecutionEngine/Orc/Core.h>
#include <llvm/ExecutionEngine/Orc/ExecutionUtils.h>
#include <llvm/ExecutionEngine/Orc/IRCompileLayer.h>
#include <llvm/ExecutionEngine/Orc/IRTransformLayer.h>
#include <llvm/ExecutionEngine/Orc/JITTargetMachineBuilder.h>
#include <llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h>
#include <llvm/ExecutionEngine/SectionMemoryManager.h>

#include <llvm/ExecutionEngine/Orc/LLJIT.h>

#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Support/TargetSelect.h>

#include "utils.hpp"

namespace codegen {

class compiler_context {
  llvm::orc::ExecutionSession session_;

  llvm::DataLayout data_layout_;

  std::unique_ptr<llvm::orc::LLJIT> lljit_;

  llvm::orc::MangleAndInterner mangle_;

  llvm::JITEventListener* gdb_listener_;

  const std::string name_;

  std::unordered_map<std::string, llvm::StructType*> custom_types;

  friend class jit_module_builder;

private:
  explicit compiler_context(std::string const& context_name, llvm::orc::JITTargetMachineBuilder tmb,
                    std::string const& name = "LLVM_JIT")
      : data_layout_(cantFail(tmb.getDefaultDataLayoutForTarget())), mangle_(session_, data_layout_),
        gdb_listener_(llvm::JITEventListener::createGDBRegistrationListener()), name_(name) {
    auto jtmb = cantFail(llvm::orc::JITTargetMachineBuilder::detectHost());
    lljit_ = cantFail(
        (llvm::orc::LLJITBuilder()
             .setJITTargetMachineBuilder(std::move(jtmb))
             .setObjectLinkingLayerCreator([&](llvm::orc::ExecutionSession& ES, const llvm::Triple& TT) {
               auto GetMemMgr = []() { return std::make_unique<llvm::SectionMemoryManager>(); };
               auto ObjLinkingLayer = std::make_unique<llvm::orc::RTDyldObjectLinkingLayer>(ES, std::move(GetMemMgr));

               // Register the event listener.
               ObjLinkingLayer->registerJITEventListener(*llvm::JITEventListener::createGDBRegistrationListener());

               // Make sure the debug info sections aren't stripped.
               ObjLinkingLayer->setProcessAllSections(true);
               return ObjLinkingLayer;
             })
             .create()));

    lljit_->getMainJITDylib().addGenerator(
        // TODO: should we expose all symbols to JIT?
        cantFail(llvm::orc::DynamicLibrarySearchGenerator::GetForCurrentProcess(
            data_layout_.getGlobalPrefix(),
            [MainName = mangle_("main")](const llvm::orc::SymbolStringPtr& Name) { return Name != MainName; })));
  }

public:
  compiler_context(std::string context_name = "codegen")
      : compiler_context(context_name, cantFail(llvm::orc::JITTargetMachineBuilder::detectHost())) {}

  compiler_context(compiler_context const&) = delete;
  compiler_context(compiler_context&&) = delete;

  void add_symbol(std::string const& name, void* address) {
    cantFail(lljit_->getMainJITDylib().define(llvm::orc::absoluteSymbols(
        {{lljit_->mangleAndIntern(std::move(name)), llvm::JITEvaluatedSymbol::fromPointer(address)}})));
  }

  llvm::Error compileModule(std::unique_ptr<llvm::Module> module, std::unique_ptr<llvm::LLVMContext> context) {
    return lljit_->addIRModule(llvm::orc::ThreadSafeModule(std::move(module), std::move(context)));
  }

  const std::string& name() { return name_; }
};

} // namespace codegen
