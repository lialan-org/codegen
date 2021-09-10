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

#include <fmt/format.h>
#include <fmt/ostream.h>

#include "compiler.hpp"
#include "types.hpp"


namespace codegen {

class compiler;
class module;

template<typename ReturnType, typename... Arguments> class function_ref {
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
    : compiler_(&c), context_(std::make_unique<llvm::LLVMContext>()),
      module_(std::make_unique<llvm::Module>(name, *context_)), ir_builder_(*context_),
      source_file_(c.source_directory_ / (name + ".txt")), dbg_builder_(*module_),
      dbg_file_(dbg_builder_.createFile(source_file_.string(), source_file_.parent_path().string())),
      dbg_scope_(dbg_file_) {
    dbg_builder_.createCompileUnit(llvm::dwarf::DW_LANG_C_plus_plus, dbg_file_, "codegen", true, "", 0);
  }

  module_builder(module_builder const&) = delete;
  module_builder(module_builder&&) = delete;

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

    //throw_on_error(compiler_->optimize_layer_.add(compiler_->session_.getMainJITDylib(),
    //                                              llvm::orc::ThreadSafeModule(std::move(module_), std::move(context_))));
    return module{compiler_->session_, compiler_->data_layout_};
  }

  friend std::ostream& operator<<(std::ostream& os, module_builder const& mb) {
    //auto llvm_os = llvm::raw_os_ostream(os);
    //mb.module_->print(llvm_os, nullptr);
    return os;
  }

private:
  void set_function_attributes(llvm::Function* func) {}

  void declare_external_symbol(std::string const& name, void* address) {
    compiler_->add_symbol(name, address);
  }
};

namespace detail {

inline thread_local module_builder* current_builder;

template<typename Type> struct type {
  static_assert(std::is_integral_v<Type>);
  static constexpr size_t alignment = alignof(Type);
  static llvm::DIType* dbg() {
    return current_builder->dbg_builder_.createBasicType(
        name(), sizeof(Type) * 8, std::is_signed_v<Type> ? llvm::dwarf::DW_ATE_signed : llvm::dwarf::DW_ATE_unsigned);
  }
  static llvm::Type* llvm() { return llvm::Type::getIntNTy(*current_builder->context_, sizeof(Type) * 8); }
  static std::string name() { return fmt::format("{}{}", std::is_signed_v<Type> ? 'i' : 'u', sizeof(Type) * 8); }
};

template<> struct type<void> {
  static constexpr size_t alignment = 0;
  static llvm::DIType* dbg() { return nullptr; }
  static llvm::Type* llvm() { return llvm::Type::getVoidTy(*current_builder->context_); }
  static std::string name() { return "void"; }
};

template<> struct type<bool> {
  static constexpr size_t alignment = alignof(bool);
  static llvm::DIType* dbg() {
    return current_builder->dbg_builder_.createBasicType(name(), 8, llvm::dwarf::DW_ATE_boolean);
  }
  static llvm::Type* llvm() { return llvm::Type::getInt1Ty(*current_builder->context_); }
  static std::string name() { return "bool"; }
};

template<> struct type<std::byte> {
  static constexpr size_t alignment = 1;
  static llvm::DIType* dbg() {
    return current_builder->dbg_builder_.createBasicType(name(), 8, llvm::dwarf::DW_ATE_unsigned);
  }
  static llvm::Type* llvm() { return llvm::Type::getInt8Ty(*current_builder->context_); }
  static std::string name() { return "byte"; }
};

template<> struct type<float> {
  static constexpr size_t alignment = alignof(float);
  static llvm::DIType* dbg() {
    return current_builder->dbg_builder_.createBasicType(name(), 32, llvm::dwarf::DW_ATE_float);
  }
  static llvm::Type* llvm() { return llvm::Type::getFloatTy(*current_builder->context_); }
  static std::string name() { return "f32"; }
};

template<> struct type<double> {
  static constexpr size_t alignment = alignof(double);
  static llvm::DIType* dbg() {
    return current_builder->dbg_builder_.createBasicType(name(), 64, llvm::dwarf::DW_ATE_float);
  }
  static llvm::Type* llvm() { return llvm::Type::getDoubleTy(*current_builder->context_); }
  static std::string name() { return "f64"; }
};

template<typename Type> struct type<Type*> {
  static constexpr size_t alignment = alignof(Type*);
  static llvm::DIType* dbg() {
    return current_builder->dbg_builder_.createPointerType(type<std::remove_cv_t<Type>>::dbg(), sizeof(Type*) * 8);
  }
  static llvm::Type* llvm() { return type<std::remove_cv_t<Type>>::llvm()->getPointerTo(); }
  static std::string name() { return type<std::remove_cv_t<Type>>::name() + '*'; }
};

// array type
template<typename Type, size_t N> struct type<Type[N]> {
  using ElementType = Type; // grab the underlying type
  static constexpr size_t alignment = alignof(Type);

  static llvm::DIType* dbg() {
    //return current_builder->dbg_builder_.createArrayType(N, alignment * 8, type<Type>::dbg(), llvm::DINodeArray{0, N - 1});
    // TODO: don't know how to create DI Type for array, need to fix this.
    return current_builder->dbg_builder_.createBasicType(name(), 32, llvm::dwarf::DW_ATE_float);
  }
  static llvm::Type* llvm() {
    return llvm::ArrayType::get(type<Type>::llvm(), N);
  }
  static std::string name() { return fmt::format("{}[{}]", type<Type>::name(), N); }
};

template<typename Type>
llvm::Value* get_constant(Type v) {
  if constexpr (std::is_integral_v<Type>) {
    return llvm::ConstantInt::get(*current_builder->context_, llvm::APInt(sizeof(Type) * 8, v, std::is_signed_v<Type>));
  } else if constexpr (std::is_floating_point_v<Type>) {
    return llvm::ConstantFP::get(*current_builder->context_, llvm::APFloat(v));
  } else if constexpr (std::is_same_v<Type, bool>) {
    return llvm::ConstantInt::get(*current_builder->context_, llvm::APInt(1, v, true));
  } else {
    static_assert(false_v<Type>, "unsupported types");
  }
}

} // namespace detail

template<typename Type> class value {
  llvm::Value* value_;
  std::string name_;

public:
  static_assert(!std::is_const_v<Type>);
  static_assert(!std::is_volatile_v<Type>);

  explicit value(llvm::Value* v, std::string const& n) : value_(v), name_(n) {}

  value(value const&) = default;
  value(value&&) = default;
  void operator=(value const&) = delete;
  void operator=(value&&) = delete;

  using value_type = Type;

  operator llvm::Value*() const noexcept { return value_; }

  llvm::Value* eval() const { return value_; }

  friend std::ostream& operator<<(std::ostream& os, value v) { return os << v.name_; }
};

template<typename Type>
value<Type> constant(Type v) requires std::is_arithmetic_v<Type> {
  static_assert(!std::is_const_v<Type>);
  return value<Type>{detail::get_constant<Type>(v), [&] {
                       if constexpr (std::is_same_v<Type, bool>) {
                         return v ? "true" : "false";
                       } else {
                         return std::to_string(v);
                       }
                     }()};
}

inline value<bool> true_() {
  return constant(true);
}
inline value<bool> false_() {
  return constant(false);
}

inline void return_() {
  auto& mb = *detail::current_builder;
  auto line_no = mb.source_code_.add_line("return;");
  mb.exited_block_ = true;
  //mb.ir_builder_.SetCurrentDebugLocation(llvm::DebugLoc::get(line_no, 1, mb.dbg_scope_));
  mb.ir_builder_.CreateRetVoid();
}

namespace detail {

template<typename FromValue, typename ToType> class bit_cast_impl {
  FromValue from_value_;

  using from_type = typename FromValue::value_type;

public:
  static_assert(sizeof(from_type) == sizeof(ToType));
  static_assert(std::is_pointer_v<from_type> == std::is_pointer_v<ToType>);

  using value_type = ToType;

  bit_cast_impl(FromValue fv) : from_value_(fv) {}

  llvm::Value* eval() {
    return detail::current_builder->ir_builder_.CreateBitCast(from_value_.eval(), type<ToType>::llvm());
  }

  friend std::ostream& operator<<(std::ostream& os, bit_cast_impl bci) {
    return os << "bit_cast<" << type<ToType>::name() << ">(" << bci.from_value_ << ")";
  }
};

template<typename FromValue, typename ToType> class cast_impl {
  FromValue from_value_;

  using from_type = typename FromValue::value_type;
  using to_type = ToType;

public:
  static_assert(!std::is_pointer_v<from_type> && !std::is_pointer_v<ToType>);

  using value_type = ToType;

  cast_impl(FromValue fv) : from_value_(fv) {}

  llvm::Value* eval() {
    auto& mb = *current_builder;
    if constexpr (std::is_floating_point_v<from_type> && std::is_floating_point_v<to_type>) {
      return mb.ir_builder_.CreateFPCast(from_value_.eval(), type<to_type>::llvm());
    } else if constexpr (std::is_floating_point_v<from_type> && std::is_integral_v<to_type>) {
      if constexpr (std::is_signed_v<to_type>) {
        return mb.ir_builder_.CreateFPToSI(from_value_.eval(), type<to_type>::llvm());
      } else {
        return mb.ir_builder_.CreateFPToUI(from_value_.eval(), type<to_type>::llvm());
      }
    } else if constexpr (std::is_integral_v<from_type> && std::is_floating_point_v<to_type>) {
      if constexpr (std::is_signed_v<from_type>) {
        return mb.ir_builder_.CreateSIToFP(from_value_.eval(), type<to_type>::llvm());
      } else {
        return mb.ir_builder_.CreateUIToFP(from_value_.eval(), type<to_type>::llvm());
      }
    } else if constexpr (std::is_integral_v<from_type> && std::is_integral_v<to_type>) {
      if constexpr (std::is_signed_v<from_type>) {
        return mb.ir_builder_.CreateSExtOrTrunc(from_value_.eval(), type<to_type>::llvm());
      } else {
        return mb.ir_builder_.CreateZExtOrTrunc(from_value_.eval(), type<to_type>::llvm());
      }
    }
  }

  friend std::ostream& operator<<(std::ostream& os, cast_impl ci) {
    return os << "cast<" << type<ToType>::name() << ">(" << ci.from_value_ << ")";
  }
};

} // namespace detail

template<typename ToType, typename FromValue> auto bit_cast(FromValue v) {
  return detail::bit_cast_impl<FromValue, ToType>(v);
}

template<typename ToType, typename FromValue> auto cast(FromValue v) {
  return detail::cast_impl<FromValue, ToType>(v);
}

void return_();

template<typename Value> void return_(Value v) {
  auto& mb = *detail::current_builder;
  mb.exited_block_ = true;
  auto line_no = mb.source_code_.add_line(fmt::format("return {};", v));
  //mb.ir_builder_.SetCurrentDebugLocation(llvm::DebugLoc::get(line_no, 1, mb.dbg_scope_));
  mb.ir_builder_.CreateRet(v.eval());
}

namespace detail {

template<typename> class function_builder;

template<typename ReturnType, typename... Arguments> class function_builder<ReturnType(Arguments...)> {
  template<typename Argument> void prepare_argument(llvm::Function::arg_iterator args, size_t idx) {
    auto& mb = *current_builder;

    auto it = args + idx;
    auto name = "arg" + std::to_string(idx);
    it->setName(name);

    auto dbg_arg = mb.dbg_builder_.createParameterVariable(mb.dbg_scope_, name, idx + 1, mb.dbg_file_,
                                                           mb.source_code_.current_line(), type<Argument>::dbg());
                                                           
    //mb.dbg_builder_.insertDbgValueIntrinsic(&*(args + idx), dbg_arg, mb.dbg_builder_.createExpression(),
    //                                        llvm::DebugLoc::get(mb.source_code_.current_line(), 1, mb.dbg_scope_),
    //                                        mb.ir_builder_.GetInsertBlock());
  }

  template<size_t... Idx, typename FunctionBuilder>
  void call_builder(std::index_sequence<Idx...>, std::string const& name, FunctionBuilder&& fb,
                    llvm::Function::arg_iterator args) {
    auto& mb = *current_builder;

    auto str = std::stringstream{};
    str << type<ReturnType>::name() << " " << name << "(";
    (void)(str << ...
               << (type<Arguments>::name() + " arg" + std::to_string(Idx) + (Idx + 1 == sizeof...(Idx) ? "" : ", ")));
    str << ") {";
    mb.source_code_.add_line(str.str());
    mb.source_code_.enter_scope();

    [[maybe_unused]] auto _ = {0, (prepare_argument<Arguments>(args, Idx), 0)...};
    fb(value<Arguments>(&*(args + Idx), "arg" + std::to_string(Idx))...);

    mb.source_code_.leave_scope();
    mb.source_code_.add_line("}");
  }

public:
  template<typename FunctionBuilder>
  function_ref<ReturnType, Arguments...> operator()(std::string const& name, FunctionBuilder&& fb) {
    auto& mb = *current_builder;
    auto fn_type = llvm::FunctionType::get(type<ReturnType>::llvm(), {type<Arguments>::llvm()...}, false);
    auto fn = llvm::Function::Create(fn_type, llvm::GlobalValue::LinkageTypes::ExternalLinkage, name, mb.module_.get());

    std::vector<llvm::Metadata*> dbg_types = {detail::type<ReturnType>::dbg(), detail::type<Arguments>::dbg()...};
    auto dbg_fn_type = mb.dbg_builder_.createSubroutineType(mb.dbg_builder_.getOrCreateTypeArray(dbg_types));
    auto dbg_fn_scope = mb.dbg_builder_.createFunction(
        mb.dbg_scope_, name, name, mb.dbg_file_, mb.source_code_.current_line(), dbg_fn_type,
        mb.source_code_.current_line(), llvm::DINode::FlagPrototyped,
        llvm::DISubprogram::DISPFlags::SPFlagDefinition | llvm::DISubprogram::DISPFlags::SPFlagOptimized);
    auto parent_scope = std::exchange(mb.dbg_scope_, dbg_fn_scope);
    fn->setSubprogram(dbg_fn_scope);

    mb.ir_builder_.SetCurrentDebugLocation(llvm::DebugLoc{});

    auto block = llvm::BasicBlock::Create(*mb.context_, "entry", fn);
    mb.ir_builder_.SetInsertPoint(block);

    mb.function_ = fn;
    call_builder(std::index_sequence_for<Arguments...>{}, name, fb, fn->arg_begin());

    mb.dbg_scope_ = parent_scope;

    return function_ref<ReturnType, Arguments...>{name, fn};
  }
};

} // namespace detail

template<typename FunctionType, typename FunctionBuilder>
auto module_builder::create_function(std::string const& name, FunctionBuilder&& fb) {
  assert(detail::current_builder == this || !detail::current_builder);
  auto prev_builder = std::exchange(detail::current_builder, this);
  exited_block_ = false;
  auto fn_ref = detail::function_builder<FunctionType>{}(name, fb);
  set_function_attributes(fn_ref);
  detail::current_builder = prev_builder;
  return fn_ref;
}

namespace detail {

template<typename> class function_declaration_builder;

template<typename ReturnType, typename... Arguments> class function_declaration_builder<ReturnType(Arguments...)> {
public:
  function_ref<ReturnType, Arguments...> operator()(std::string const& name) {
    auto& mb = *current_builder;

    auto fn_type = llvm::FunctionType::get(type<ReturnType>::llvm(), {type<Arguments>::llvm()...}, false);
    auto fn = llvm::Function::Create(fn_type, llvm::GlobalValue::LinkageTypes::ExternalLinkage, name, mb.module_.get());

    return function_ref<ReturnType, Arguments...>{name, fn};
  }
};

} // namespace detail

template<typename FunctionType>
auto module_builder::declare_external_function(std::string const& name, FunctionType* fn) {
  assert(detail::current_builder == this || !detail::current_builder);

  auto prev_builder = std::exchange(detail::current_builder, this);
  auto fn_ref = detail::function_declaration_builder<FunctionType>{}(name);
  detail::current_builder = prev_builder;

  declare_external_symbol(name, reinterpret_cast<void*>(fn));

  return fn_ref;
}

} // namespace codegen
