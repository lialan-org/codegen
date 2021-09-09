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

#include "codegen/compiler.hpp"

#include "os.hpp"

#include <random>

#include <llvm/Analysis/TargetLibraryInfo.h>
#include <llvm/Analysis/TargetTransformInfo.h>

#include <llvm/ExecutionEngine/SectionMemoryManager.h>

#include <llvm/ExecutionEngine/Orc/CompileUtils.h>
#include <llvm/ExecutionEngine/Orc/ExecutionUtils.h>

#include <llvm/Support/TargetSelect.h>

#include "codegen/module.hpp"

namespace codegen {

compiler::compiler(llvm::orc::JITTargetMachineBuilder tmb)
    : data_layout_(unwrap(tmb.getDefaultDataLayoutForTarget())), target_machine_(unwrap(tmb.createTargetMachine())),
      mangle_(session_, data_layout_) {}

compiler::compiler()
    : compiler([] {
        llvm::InitializeNativeTarget();
        llvm::InitializeNativeTargetAsmPrinter();

        auto tmb = unwrap(llvm::orc::JITTargetMachineBuilder::detectHost());
        tmb.setCodeGenOptLevel(llvm::CodeGenOpt::Aggressive);
        //tmb.setCPU(llvm::sys::getHostCPUName());
        return tmb;
      }()) { }

compiler::~compiler() {
  //for (auto vk : loaded_modules_) { gdb_listener_->notifyFreeingObject(vk); }
  std::filesystem::remove_all(source_directory_);
}

llvm::Expected<llvm::orc::ThreadSafeModule> compiler::optimize_module(llvm::orc::ThreadSafeModule tsm,
                                                                      llvm::orc::MaterializationResponsibility const&) {
}

void compiler::add_symbol(std::string const& name, void* address) {
  //external_symbols_[*mangle_(name)] = reinterpret_cast<uintptr_t>(address);
}

module::module(llvm::orc::ExecutionSession& session, llvm::DataLayout const& dl)
    : session_(&session), mangle_(session, dl) {
}

void* module::get_address(std::string const& name) {
  //auto address = unwrap(session_->lookup({&session_->getMainJITDylib()}, mangle_(name))).getAddress();
  //return reinterpret_cast<void*>(address);
}

} // namespace codegen
