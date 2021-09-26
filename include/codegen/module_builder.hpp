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

#include <fstream>
#include <sstream>
#include <string>

#include <llvm/IR/DIBuilder.h>
#include <llvm/IR/DebugInfoMetadata.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>

#include <llvm/Support/Host.h>
#include <llvm/Support/raw_os_ostream.h>

#include <fmt/format.h>
#include <fmt/ostream.h>

#include "compiler_context.hpp"
#include "module.hpp"

namespace codegen {

class compiler_context;
class module;

template<typename ReturnType, typename... Arguments> class function_ref {
  std::string name_;
  llvm::Function* function_;

public:
  explicit function_ref(std::string const& name, llvm::Function* fn) : name_(name), function_(fn) {}

  void set_function_attribute(std::pair<llvm::StringRef, llvm::StringRef> attribute_set) {
    function_->addFnAttr(attribute_set.first, attribute_set.second);
  }

  operator llvm::FunctionCallee() const { return function_; }

  operator llvm::Function *() const { return function_; }

  std::string const& name() const { return name_; }
};

class module_builder {
  compiler_context* compiler_;
  std::unique_ptr<llvm::LLVMContext> context_;
  llvm::IRBuilder<> ir_builder_;
  std::unique_ptr<llvm::Module> module_;
  llvm::Function* function_;

  static inline thread_local std::unique_ptr<module_builder> current_builder_;

public:
  class source_code_generator {
    std::stringstream source_code_;
    unsigned line_no_ = 1;
    unsigned indent_ = 0;

    std::filesystem::path source_file_;
    llvm::Module& module_;
    llvm::DIBuilder dbg_builder_;
    llvm::DIFile* dbg_file_;
    std::stack<llvm::DIScope*> dbg_scopes_;

  public:
    source_code_generator(llvm::Module& module, std::filesystem::path source_file)
        : source_file_(source_file), module_(module), dbg_builder_(module_),
          dbg_file_(dbg_builder_.createFile(source_file.string(), source_file.parent_path().string())),
          dbg_scopes_({dbg_file_}) {
      dbg_builder_.createCompileUnit(llvm::dwarf::DW_LANG_C_plus_plus, dbg_file_, "codegen", true, "", 0);
    }

    llvm::DIBuilder& debug_builder() { return dbg_builder_; }
    llvm::DIFile* debug_file() { return dbg_file_; }
    llvm::DIScope* debug_scope() { return dbg_scopes_.top(); }
    void create_debug_scope(llvm::DIScope* new_scope) { dbg_scopes_.push(new_scope); }

    llvm::DILocation* get_debug_location(unsigned line, unsigned col = 1) {
      return llvm::DILocation::get(module_.getContext(), line, col, debug_scope());
    }

    unsigned add_line(std::string const& line) {
      source_code_ << std::string(indent_, ' ') << line << "\n";
      return line_no_++;
    }

    void enter_scope() {
      indent_ += 4;
      dbg_scopes_.emplace(dbg_builder_.createLexicalBlock(debug_scope(), dbg_file_, current_line(), 1));
    }

    void leave_scope() {
      indent_ -= 4;
      dbg_scopes_.pop();
    }

    template<typename ReturnType, typename... Arguments>
    llvm::DISubprogram* enter_function_scope(std::string const& function_name);

    void leave_function_scope() { dbg_scopes_.pop(); }

    unsigned current_line() const { return line_no_; }

    std::string get() const { return source_code_.str(); }

    std::filesystem::path source_file() { return source_file_; }

    friend std::ostream& operator<<(std::ostream& os, source_code_generator const& scg) {
      auto llvm_os = llvm::raw_os_ostream(os);
      llvm_os << scg.get();
      return os;
    }
  };

  source_code_generator source_code_;

  struct loop {
    llvm::BasicBlock* continue_block_ = nullptr;
    llvm::BasicBlock* break_block_ = nullptr;
  };
  loop current_loop_;
  bool exited_block_ = false;

  static void register_current_builder(module_builder* builder) { module_builder::current_builder_.reset(builder); }

  static void deregister_current_builder() { module_builder::current_builder_.release(); }

public:
  module_builder(compiler_context& c, std::string const& name, bool enable_debug_codegen = true)
      : compiler_(&c), context_(std::make_unique<llvm::LLVMContext>()), ir_builder_(*context_),
        module_(std::make_unique<llvm::Module>(name, *context_)), function_(nullptr),
        source_code_(*module_,
                     std::filesystem::temp_directory_path() / ("cg_" + c.name()) / std::filesystem::path(name + ".c")) {
    std::filesystem::create_directories(source_code_.source_file().parent_path());

    assert(module_builder::current_builder_.get() == nullptr);
    module_builder::register_current_builder(this);
  }

  ~module_builder() { module_builder::deregister_current_builder(); }

  static module_builder* current_builder() { return module_builder::current_builder_.get(); }

  llvm::DIBuilder& debug_builder() { return source_code_.debug_builder(); }

  llvm::DILocation* get_debug_location(unsigned line, unsigned col = 1) {
    return source_code_.get_debug_location(line, col);
  }

  module_builder(module_builder const&) = delete;
  module_builder(module_builder&&) = delete;
  module_builder& operator=(const module_builder) = delete;

  template<typename FunctionType, typename FunctionBuilder>
  auto create_function(std::string const& name, FunctionBuilder&& fb);

  template<typename FunctionType> auto declare_external_function(std::string const& name, FunctionType* fn);

  void dump_llvm_ir(llvm::raw_ostream& out) const { module_->print(out, nullptr); }

  llvm::IRBuilder<>& ir_builder() { return ir_builder_; }
  llvm::LLVMContext& context() { return *context_; }
  llvm::Module& module() { return *module_; }

  llvm::Function*& current_function() { return function_; }

  [[nodiscard]] class module build() && {
    {
      auto ofs = std::ofstream(source_code_.source_file(), std::ios::trunc);
      ofs << source_code_.get();
    }
    source_code_.debug_builder().finalize();

    if (compiler_->compileModule(std::move(module_), std::move(context_))) {
      // TODO: clean up module builder.
    } else {
      // llvm_unreachable("Failed to compile"); // TODO: more error messages.
    }

    return codegen::module{std::move(compiler_->lljit_), compiler_->mangle_};
  }

  friend std::ostream& operator<<(std::ostream& os, module_builder const& mb) {
    auto llvm_os = llvm::raw_os_ostream(os);
    mb.module_->print(llvm_os, nullptr);
    return os;
  }

private:

  void declare_external_symbol(std::string const& name, void* address) { compiler_->add_symbol(name, address); }
};

} // namespace codegen
