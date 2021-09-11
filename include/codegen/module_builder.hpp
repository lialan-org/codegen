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
#include <sstream>
#include <fstream>
#include <string>

#include <llvm/IR/DIBuilder.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/DebugInfoMetadata.h>

#include <llvm/Support/raw_os_ostream.h>

#include <fmt/format.h>
#include <fmt/ostream.h>

#include "compiler.hpp"

namespace codegen {

class compiler;
class module;

template<typename ReturnType, typename... Arguments>
class function_ref {
  std::string name_;
  llvm::Function* function_;

public:
  explicit function_ref(std::string const& name, llvm::Function* fn) : name_(name), function_(fn) {}

  operator llvm::Function*() const { return function_; }

  std::string const& name() const { return name_; }
};

class module_builder {
  compiler* compiler_;

public: // FIXME: proper encapsulation
  std::unique_ptr<llvm::LLVMContext> context_;
  std::unique_ptr<llvm::Module> module_;

  llvm::IRBuilder<> ir_builder_;

  llvm::Function* function_{};

  class source_code_generator {
    std::stringstream source_code_;
    unsigned line_no_ = 1;
    unsigned indent_ = 0;

  public:
    unsigned add_line(std::string const& line) {
      source_code_ << std::string(indent_, ' ') << line << "\n";
      return line_no_++;
    }

    void enter_scope() { indent_ += 4; }
    void leave_scope() { indent_ -= 4; }
    unsigned current_line() const { return line_no_; }
    std::string get() const {
      return source_code_.str();
    }
  };

  source_code_generator source_code_;
  std::filesystem::path source_file_;

  struct loop {
    llvm::BasicBlock* continue_block_ = nullptr;
    llvm::BasicBlock* break_block_ = nullptr;
  };
  loop current_loop_;
  bool exited_block_ = false;

  llvm::DIBuilder dbg_builder_;

  llvm::DIFile* dbg_file_;
  llvm::DIScope* dbg_scope_;



public:
  module_builder(compiler& c, std::string const& name)
    : compiler_(&c),
      context_(std::make_unique<llvm::LLVMContext>()),
      module_(std::make_unique<llvm::Module>(name, *context_)),
      ir_builder_(*context_),
      source_file_(c.source_directory_ / (name + ".txt")),
      dbg_builder_(*module_),
      dbg_file_(dbg_builder_.createFile(source_file_.string(), source_file_.parent_path().string())),
      dbg_scope_(dbg_file_)
  {
    dbg_builder_.createCompileUnit(llvm::dwarf::DW_LANG_C_plus_plus, dbg_file_, "codegen", true, "", 0);
  }

  module_builder(module_builder const&) = delete;
  module_builder(module_builder&&) = delete;
  module_builder& operator=(const module_builder) = delete;

  template<typename FunctionType, typename FunctionBuilder>
  auto create_function(std::string const& name, FunctionBuilder&& fb);

  template<typename FunctionType> auto declare_external_function(std::string const& name, FunctionType* fn);

  [[nodiscard]] module build() && {
    {
      auto ofs = std::ofstream(source_file_, std::ios::trunc);
      ofs << source_code_.get();
    }

    dbg_builder_.finalize();

    module_->print(llvm::errs(), nullptr);

    auto target_triple = compiler_->target_machine_->getTargetTriple();
    module_->setDataLayout(compiler_->data_layout_);
    module_->setTargetTriple(target_triple.str());

    // TODO: use a custom resource tracker
    throw_on_error(compiler_->optimize_layer_.add(compiler_->getMainJITDylib().getDefaultResourceTracker(),
                                                  llvm::orc::ThreadSafeModule(std::move(module_), std::move(context_))));
    return module{compiler_->session_, compiler_->data_layout_};
  }

  friend std::ostream& operator<<(std::ostream& os, module_builder const& mb) {
    //auto llvm_os = llvm::raw_os_ostream(os);
    //mb.module_->print(llvm_os, nullptr);
    return os;
  }

private:
  void set_function_attributes(llvm::Function* fn) {
    fn->addFnAttr("target-cpu", LLVMGetHostCPUName());
  }

  void declare_external_symbol(std::string const& name, void* address) {
    compiler_->add_symbol(name, address);
  }
};

namespace detail {

inline thread_local module_builder* current_builder;

} // namespace detail

} // namespace codegen
