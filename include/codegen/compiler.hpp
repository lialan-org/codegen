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

#include <llvm/ExecutionEngine/Orc/Core.h>
#include <llvm/ExecutionEngine/Orc/ExecutionUtils.h>
#include <llvm/ExecutionEngine/Orc/IRCompileLayer.h>
#include <llvm/ExecutionEngine/Orc/IRTransformLayer.h>
#include <llvm/ExecutionEngine/Orc/JITTargetMachineBuilder.h>
#include <llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h>

#include "module.hpp"
#include "utils.hpp"

namespace codegen {

class compiler {
  llvm::orc::ExecutionSession session_;

  llvm::DataLayout data_layout_;
  std::unique_ptr<llvm::TargetMachine> target_machine_;

  llvm::orc::MangleAndInterner mangle_;

  //llvm::orc::RTDyldObjectLinkingLayer object_layer_;
  //llvm::orc::IRCompileLayer compile_layer_;
  //llvm::orc::IRTransformLayer optimize_layer_;

  //llvm::JITEventListener* gdb_listener_;

  std::filesystem::path source_directory_;

  //std::vector<llvm::orc::VModuleKey> loaded_modules_;

  std::unordered_map<std::string, uintptr_t> external_symbols_;
  //llvm::orc::DynamicLibrarySearchGenerator dynlib_generator_;

  friend class module_builder;

private:
  explicit compiler(llvm::orc::JITTargetMachineBuilder tmb)
    : data_layout_(unwrap(tmb.getDefaultDataLayoutForTarget())), target_machine_(unwrap(tmb.createTargetMachine())),
      mangle_(session_, data_layout_) {}

public:
  compiler()
    : compiler([] {
        LLVMInitializeNativeTarget();
        LLVMInitializeNativeAsmPrinter();

        auto tmb = unwrap(llvm::orc::JITTargetMachineBuilder::detectHost());
        tmb.setCodeGenOptLevel(llvm::CodeGenOpt::Aggressive);
        //tmb.setCPU(llvm::sys::getHostCPUName());
        return tmb;
      }()) { }

  ~compiler() {
  //for (auto vk : loaded_modules_) { gdb_listener_->notifyFreeingObject(vk); }
  std::filesystem::remove_all(source_directory_);
}

  compiler(compiler const&) = delete;
  compiler(compiler&&) = delete;

  void add_symbol(std::string const& name, void* address) {}

private:
  llvm::Expected<llvm::orc::ThreadSafeModule> optimize_module(llvm::orc::ThreadSafeModule,
                                                              llvm::orc::MaterializationResponsibility const&) {}
};

} // namespace codegen
