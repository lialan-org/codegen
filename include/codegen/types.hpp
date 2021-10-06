#pragma once

#include "module_builder.hpp"

#include "types_detail.hpp"

#include <sstream>

#include <concepts>

namespace codegen {

class value {
protected:
  llvm::Value* value_;
  std::string name_;

public:
  explicit value(llvm::Value* v, std::string const& n)
    : value_(v), name_(n) {}

  value(value const&) = default;
  value(value&&) = default;
  void operator=(value const&) = delete;
  void operator=(value&&) = delete;

  virtual operator llvm::Value *() const noexcept { return value_; }

  virtual llvm::Type* get_type() const { return value_->getType(); }

  virtual llvm::Value* eval() const { return value_; }
  virtual std::string get_name() const { return name_; }

  friend std::ostream& operator<<(std::ostream& os, value v) { return os << v.get_name(); }

  bool isPointerType() const {
    return get_type()->isPointerTy();
  }
  bool isIntegerType() const {
    return get_type()->isIntegerTy();
  }
  bool isBoolType() const {
    return get_type()->isIntegerTy(1);
  }
  bool isFloatType() const {
    return get_type()->isFloatTy();
  }

  // TODO: LLVM IR does not distinguish sign or unsigned values. so we need to carry
  // extra info.
  /*
  bool isSignedIntegerType() const { }
  */

  bool isPointerElementType() const {
    assert(isPointerType());
    llvm::PointerType * ptr_type = llvm::dyn_cast<llvm::PointerType>(get_type());
    return ptr_type->getElementType();
  }
};

void jit_module_builder::prepare_function_arguments(llvm::Function *fn) {
  auto& mb = *codegen::jit_module_builder::current_builder();
  auto& debug_builder = mb.debug_builder();

  for (size_t idx = 0; idx < fn->arg_size(); ++idx)  {
    auto it = fn->getArg(idx);
    auto name = "arg" + std::to_string(idx);
    it->setName(name);

    auto dbg_arg = debug_builder.createParameterVariable(mb.source_code_.debug_scope(), name, idx + 1,
                                                         mb.source_code_.debug_file(), mb.source_code_.current_line(),
                                                         detail::type_reverse_lookup::dbg(it->getType()));
    mb.debug_builder().insertDbgValueIntrinsic(&*it, dbg_arg, debug_builder.createExpression(),
                                               mb.get_debug_location(mb.source_code_.current_line()),
                                               mb.ir_builder().GetInsertBlock());
  }
}

} // namespace codegen

namespace codegen::detail {

template<typename Type>
inline llvm::Value* get_constant(Type v) {
  if constexpr (std::is_same_v<Type, bool>) {
    return llvm::ConstantInt::get(codegen::jit_module_builder::current_builder()->context(), llvm::APInt(1, v, true));
  } else if constexpr (std::is_integral_v<Type>) {
    return llvm::ConstantInt::get(codegen::jit_module_builder::current_builder()->context(),
                                  llvm::APInt(sizeof(Type) * 8, v, std::is_signed_v<Type>));
  } else if constexpr (std::is_floating_point_v<Type>) {
    return llvm::ConstantFP::get(codegen::jit_module_builder::current_builder()->context(), llvm::APFloat(v));
  } else {
    llvm_unreachable("Unsupported type");
  }
}

class function_declaration_builder {
  // TODO: check if we need to carry runtime type inside function_ref
public:
  codegen::function_ref operator()(std::string const& name, llvm::FunctionType *func_type) {
    auto& mb = *codegen::jit_module_builder::current_builder();
    auto fn = llvm::Function::Create(func_type, llvm::GlobalValue::LinkageTypes::ExternalLinkage, name, mb.module());
    return codegen::function_ref{name, fn};
  }
};

} // namespace codegen::detail

namespace codegen {

template<typename Type>
inline value constant(Type v) {
  return value{detail::get_constant<Type>(v), [&] {
                       if constexpr (std::is_same_v<Type, bool>) {
                         return v ? "true" : "false";
                       } else {
                         return std::to_string(v);
                       }
                     }()};
}

inline void jit_module_builder::begin_creating_function(std::string const& name, llvm::FunctionType* func_type) {
  assert(jit_module_builder::current_builder() == this || !jit_module_builder::current_builder());
  auto& mb = *codegen::jit_module_builder::current_builder();

  assert(!mb.current_function() && "Cannot create function inside function");

  auto fn = llvm::Function::Create(func_type, llvm::GlobalValue::LinkageTypes::ExternalLinkage, name, mb.module());
  mb.current_function() = fn;

  auto dbg_fn_scope = mb.source_code_.jit_enter_function_scope(name, func_type);
  fn->setSubprogram(dbg_fn_scope);
  mb.ir_builder().SetCurrentDebugLocation(mb.get_debug_location(mb.source_code_.current_line()));

  auto block = llvm::BasicBlock::Create(mb.context(), "entry", fn);
  mb.ir_builder().SetInsertPoint(block);

  auto str = std::stringstream{};
  str << detail::type_reverse_lookup::name(func_type->getReturnType()) << " " << name << "(";
  auto params = func_type->params();
  for (size_t i = 0; i < params.size(); i++) {
    str << detail::type_reverse_lookup::name(params[i]) + " arg" + std::to_string(i);
    if (i != params.size() - 1) {
      str << ", ";
    }
  }
  str << ") {";

  mb.source_code_.add_line(str.str());
  mb.source_code_.enter_scope();

  prepare_function_arguments(fn);
  exited_block_ = false;
}

inline function_ref jit_module_builder::end_creating_function() {
  auto& mb = *codegen::jit_module_builder::current_builder();
  mb.source_code_.leave_scope();
  mb.source_code_.add_line("}");

  mb.source_code_.leave_function_scope();
  function_ref fn_ref = function_ref{current_function_name_, mb.current_function()};
  fn_ref.set_function_attribute({"target-cpu", llvm::sys::getHostCPUName()});
  return fn_ref;
}

inline function_ref jit_module_builder::declare_external_function(std::string const& name, llvm::FunctionType* fn) {
  assert(jit_module_builder::current_builder() == this || !jit_module_builder::current_builder());

  auto fn_ref = detail::function_declaration_builder{}(name, fn);
  declare_external_symbol(name, reinterpret_cast<void*>(fn));
  return fn_ref;
}

inline llvm::DISubprogram*
jit_module_builder::source_code_generator::jit_enter_function_scope(std::string const& function_name, llvm::FunctionType* func_type) {
  auto params = func_type->params();
  llvm::SmallVector<llvm::Metadata*> dbg_types(params.size() + 1);

  auto return_type = func_type->getReturnType();
  dbg_types.push_back(detail::type_reverse_lookup::dbg(return_type));

  for (auto param : params) {
    dbg_types.push_back(detail::type_reverse_lookup::dbg(param));
  }

  auto dbg_fn_type = dbg_builder_.createSubroutineType(dbg_builder_.getOrCreateTypeArray(dbg_types));
  auto dbg_fn_scope = dbg_builder_.createFunction(
      debug_scope(), function_name, function_name, debug_file(), current_line(), dbg_fn_type, current_line(),
      llvm::DINode::FlagPrototyped,
      llvm::DISubprogram::DISPFlags::SPFlagDefinition | llvm::DISubprogram::DISPFlags::SPFlagOptimized);
  dbg_scopes_.push(dbg_fn_scope);
  return dbg_fn_scope;
}

} // namespace codegen
