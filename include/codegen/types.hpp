#pragma once

#include "module_builder.hpp"

#include "types_detail.hpp"

#include <sstream>

#include <concepts>

namespace codegen {

template<class... T> constexpr bool always_false = false;

template<typename T> concept SupportedLLVMType = requires(T t) {
  codegen::detail::type<T>::value_type;
};

template<typename T> concept IsPlainType = !std::is_volatile_v<T> && !std::is_reference_v<T>;

template<typename T> concept IsPointer = std::is_pointer_v<T>&& IsPlainType<T>;

template<typename T> concept LLVMBoolType = IsPlainType<T>&& std::same_as<T, bool>;

template<typename T> concept LLVMVoidType = IsPlainType<T>&& std::same_as<T, void>;

template<typename T>
concept LLVMIntegralType = IsPlainType<T> &&
                           (std::is_integral_v<T> || std::same_as<T, int8_t> || std::same_as<T, uint8_t>);

template<typename T> concept LLVMFloatingType = IsPlainType<T>&& std::is_floating_point_v<T>;

template<typename T> concept LLVMArithmeticType = LLVMIntegralType<T> || LLVMFloatingType<T>;

template<typename T>
concept LLVMPODType = std::same_as<T, std::byte> || LLVMIntegralType<T> || LLVMVoidType<T> || LLVMFloatingType<T>;

template<typename T> concept LLVMBytePointerType = IsPointer<T>&& std::same_as<std::remove_pointer_t<T>, std::byte>;

template<typename T> concept LLVMIntegralPointerType = IsPointer<T>&& LLVMIntegralType<std::remove_pointer_t<T>>;

template<typename T> concept LLVMFloatingPointerType = IsPointer<T>&& LLVMFloatingType<std::remove_pointer_t<T>>;

template<typename T> concept LLVMVoidPointerType = IsPointer<T>&& LLVMVoidType<std::remove_pointer_t<T>>;

template<typename T>
concept LLVMPointerPointerType = IsPointer<T> && (LLVMIntegralPointerType<std::remove_pointer_t<T>> ||
                                                  LLVMFloatingPointerType<std::remove_pointer_t<T>> ||
                                                  LLVMVoidPointerType<std::remove_pointer_t<T>> ||
                                                  LLVMBytePointerType<std::remove_pointer_t<T>>);

template<typename T>
concept LLVMPointerType = LLVMIntegralPointerType<T> || LLVMFloatingPointerType<T> || LLVMVoidPointerType<T> ||
                          LLVMBytePointerType<T> || LLVMPointerPointerType<T>;

template<typename T>
concept LLVMArrayType = std::is_array_v<T>&& std::extent_v<T> != 0 && LLVMPODType<std::remove_all_extents_t<T>>;

template<typename T> concept LLVMType = LLVMPODType<T> || LLVMPointerType<T> || LLVMArrayType<T>;

template<typename T> concept LLVMTypeWrapper = requires(T t) {
  typename T::value_type;
  LLVMType<typename T::value_type>;
};

template<typename T> concept Pointer = LLVMTypeWrapper<T>&& std::is_pointer_v<typename T::value_type>;

template<typename T> concept ConditionType = std::is_same_v<typename std::decay_t<T>::value_type, bool>;

template<typename S>
concept Size = LLVMTypeWrapper<S> &&
               (std::is_same_v<typename S::value_type, int32_t> || std::is_same_v<typename S::value_type, int64_t> ||
                std::is_same_v<typename S::value_type, uint32_t> || std::is_same_v<typename S::value_type, uint64_t>);

template<typename T> concept Integral = LLVMTypeWrapper<T>&& std::is_integral_v<T>;

template<typename T> concept IsArray = std::is_array_v<T>&& LLVMType<std::remove_all_extents_t<T>>;

template<typename T> concept Bool = requires(T t) {
  LLVMTypeWrapper<T>;
  std::same_as<typename T::value_type, bool>;
};

template<typename T> concept Void = requires(T t) {
  LLVMTypeWrapper<T>;
  std::same_as<typename T::value_type, void>;
};

template<typename T> concept Byte = requires(T t) {
  LLVMTypeWrapper<T>;
  std::same_as<typename T::value_type, std::byte>;
};

template<typename T> concept Float = requires(T t) {
  LLVMTypeWrapper<T>;
  std::same_as<typename T::value_type, float>;
};

template<typename T> concept Double = requires(T t) {
  LLVMTypeWrapper<T>;
  std::same_as<typename T::value_type, double>;
};

template<LLVMType Type> class value {
  llvm::Value* value_;
  std::string name_;

public:
  explicit value(llvm::Value* v, std::string const& n) : value_(v), name_(n) {}

  value(value const&) = default;
  value(value&&) = default;
  void operator=(value const&) = delete;
  void operator=(value&&) = delete;

  using value_type = Type;

  operator llvm::Value *() const noexcept { return value_; }
  llvm::Value* eval() const { return value_; }
  friend std::ostream& operator<<(std::ostream& os, value v) { return os << v.name_; }
};

template<typename T> concept IsValue = requires(T t) {
  detail::is_instance<T, value>{};
};

template<typename T> concept IntegralValue = IsValue<T>&& LLVMIntegralType<typename T::value_type>;

template<typename T> concept FloatingValue = IsValue<T>&& LLVMFloatingType<typename T::value_type>;

template<typename T> concept PointerValue = IsValue<T>&& LLVMPointerType<typename T::value_type>;

template<typename T> concept ArithmeticValue = IsValue<T>&& LLVMArithmeticType<typename T::value_type>;

} // namespace codegen

namespace codegen::detail {

inline llvm::Value* get_constant(LLVMPODType auto v) {
  using Type = decltype(v);
  if constexpr (LLVMIntegralType<Type>) {
    return llvm::ConstantInt::get(codegen::module_builder::current_builder()->context(),
                                  llvm::APInt(sizeof(Type) * 8, v, std::is_signed_v<Type>));
  } else if constexpr (LLVMFloatingType<Type>) {
    return llvm::ConstantFP::get(codegen::module_builder::current_builder()->context(), llvm::APFloat(v));
  } else if constexpr (LLVMBoolType<Type>) {
    return llvm::ConstantInt::get(codegen::module_builder::current_builder()->context(), llvm::APInt(1, v, true));
  } else {
    llvm_unreachable("Unsupported type");
  }
}

template<typename> class function_builder;

template<typename ReturnType, typename... Arguments> class function_builder<ReturnType(Arguments...)> {

  template<typename Argument> void prepare_argument(llvm::Function::arg_iterator args, size_t idx) {
    auto& mb = *codegen::module_builder::current_builder();

    auto it = args + idx;
    auto name = "arg" + std::to_string(idx);
    it->setName(name);

    auto& debug_builder = mb.debug_builder();

    auto dbg_arg = debug_builder.createParameterVariable(mb.source_code_.debug_scope(), name, idx + 1,
                                                         mb.source_code_.debug_file(), mb.source_code_.current_line(),
                                                         type<Argument>::dbg());
    mb.debug_builder().insertDbgValueIntrinsic(&*(args + idx), dbg_arg, debug_builder.createExpression(),
                                               mb.get_debug_location(mb.source_code_.current_line()),
                                               mb.ir_builder().GetInsertBlock());
  }

  template<size_t... Idx, typename FunctionBuilder>
  void call_builder(std::index_sequence<Idx...>, std::string const& name, FunctionBuilder&& fb,
                    llvm::Function::arg_iterator args) {
    auto& mb = *codegen::module_builder::current_builder();

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
    auto& mb = *codegen::module_builder::current_builder();
    assert(!mb.current_function() && "Cannot define a new function inside another funciton");

    auto fn_type = llvm::FunctionType::get(type<ReturnType>::llvm(), {type<Arguments>::llvm()...}, false);
    auto fn = llvm::Function::Create(fn_type, llvm::GlobalValue::LinkageTypes::ExternalLinkage, name, mb.module());
    mb.current_function() = fn;

    auto dbg_fn_scope = mb.source_code_.enter_function_scope<ReturnType, Arguments...>(name);

    fn->setSubprogram(dbg_fn_scope);

    mb.ir_builder().SetCurrentDebugLocation(mb.get_debug_location(mb.source_code_.current_line()));

    auto block = llvm::BasicBlock::Create(mb.context(), "entry", fn);
    mb.ir_builder().SetInsertPoint(block);

    call_builder(std::index_sequence_for<Arguments...>{}, name, fb, fn->arg_begin());

    mb.source_code_.leave_function_scope();

    mb.current_function() = nullptr;
    return function_ref<ReturnType, Arguments...>{name, fn};
  }
};

class jit_function_builder {
  void prepare_arguments(llvm::Function *fn) {
    auto& mb = *codegen::jit_module_builder::current_builder();
    auto& debug_builder = mb.debug_builder();

    for (size_t idx = 0; idx < fn->arg_size(); ++idx)  {
      auto it = fn->getArg(idx);
      auto name = "arg" + std::to_string(idx);
      it->setName(name);

      auto dbg_arg = debug_builder.createParameterVariable(mb.source_code_.debug_scope(), name, idx + 1,
                                                           mb.source_code_.debug_file(), mb.source_code_.current_line(),
                                                           type_reverse_lookup::dbg(it->getType()));
      mb.debug_builder().insertDbgValueIntrinsic(&*it, dbg_arg, debug_builder.createExpression(),
                                                 mb.get_debug_location(mb.source_code_.current_line()),
                                                 mb.ir_builder().GetInsertBlock());
    }
  }

public:
  /// we take a llvm::FunctionType because users might need to construct it outside in their cases
  void start_creating_function(std::string const& name, llvm::FunctionType* func_type) {
    auto& mb = *codegen::jit_module_builder::current_builder();
    assert(!mb.current_function() && "Cannot define a new function inside another funciton");
    auto fn = llvm::Function::Create(func_type, llvm::GlobalValue::LinkageTypes::ExternalLinkage, name, mb.module());
    mb.current_function() = fn;

    auto dbg_fn_scope = mb.source_code_.jit_enter_function_scope(name, func_type);
    fn->setSubprogram(dbg_fn_scope);
    mb.ir_builder().SetCurrentDebugLocation(mb.get_debug_location(mb.source_code_.current_line()));

    auto block = llvm::BasicBlock::Create(mb.context(), "entry", fn);
    mb.ir_builder().SetInsertPoint(block);

    auto str = std::stringstream{};
    str << type_reverse_lookup::name(func_type->getReturnType()) << " " << name << "(";
    auto params = func_type->params();
    for (size_t i = 0; i < params.size(); i++) {
      str << type_reverse_lookup::name(params[i]) + " arg" + std::to_string(i);
      if (i != params.size() - 1) {
        str << ", ";
      }
    }
    str << ") {";

    mb.source_code_.add_line(str.str());
    mb.source_code_.enter_scope();

    prepare_arguments(fn);

    // the next step is to run:
    //fb(value<Arguments>(&*(args + Idx), "arg" + std::to_string(Idx))...);
  }

  void finish_creating_function() {
    auto& mb = *codegen::jit_module_builder::current_builder();
    mb.source_code_.leave_scope();
    mb.source_code_.add_line("}");

    mb.source_code_.leave_function_scope();
    mb.current_function() = nullptr;
  }
};

template<typename ReturnType, typename... Arguments> class function_declaration_builder<ReturnType(Arguments...)> {
public:
  function_ref<ReturnType, Arguments...> operator()(std::string const& name) {
    auto& mb = *codegen::module_builder::current_builder();

    auto fn_type = llvm::FunctionType::get(type<ReturnType>::llvm(), {type<Arguments>::llvm()...}, false);
    auto fn = llvm::Function::Create(fn_type, llvm::GlobalValue::LinkageTypes::ExternalLinkage, name, mb.module());

    return function_ref<ReturnType, Arguments...>{name, fn};
  }
};

template<typename FromValue, typename ToType> class bit_cast_impl {
  FromValue from_value_;

  using from_type = typename FromValue::value_type;

public:
  static_assert(sizeof(from_type) == sizeof(ToType));
  static_assert(std::is_pointer_v<from_type> == std::is_pointer_v<ToType>);

  using value_type = ToType;

  bit_cast_impl(FromValue fv) : from_value_(fv) {}

  llvm::Value* eval() {
    return module_builder::current_builder()->ir_builder().CreateBitCast(from_value_.eval(), type<ToType>::llvm());
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
    auto& mb = *codegen::module_builder::current_builder();
    if constexpr (std::is_floating_point_v<from_type> && std::is_floating_point_v<to_type>) {
      return mb.ir_builder().CreateFPCast(from_value_.eval(), type<to_type>::llvm());
    } else if constexpr (std::is_floating_point_v<from_type> && std::is_integral_v<to_type>) {
      if constexpr (std::is_signed_v<to_type>) {
        return mb.ir_builder().CreateFPToSI(from_value_.eval(), type<to_type>::llvm());
      } else {
        return mb.ir_builder().CreateFPToUI(from_value_.eval(), type<to_type>::llvm());
      }
    } else if constexpr (std::is_integral_v<from_type> && std::is_floating_point_v<to_type>) {
      if constexpr (std::is_signed_v<from_type>) {
        return mb.ir_builder().CreateSIToFP(from_value_.eval(), type<to_type>::llvm());
      } else {
        return mb.ir_builder().CreateUIToFP(from_value_.eval(), type<to_type>::llvm());
      }
    } else if constexpr (std::is_integral_v<from_type> && std::is_integral_v<to_type>) {
      if constexpr (std::is_signed_v<from_type>) {
        return mb.ir_builder().CreateSExtOrTrunc(from_value_.eval(), type<to_type>::llvm());
      } else {
        return mb.ir_builder().CreateZExtOrTrunc(from_value_.eval(), type<to_type>::llvm());
      }
    }
  }

  friend std::ostream& operator<<(std::ostream& os, cast_impl ci) {
    return os << "cast<" << type<ToType>::name() << ">(" << ci.from_value_ << ")";
  }
};

} // namespace codegen::detail

namespace codegen {

template<LLVMArithmeticType Type> inline value<Type> constant(Type v) {
  return value<Type>{detail::get_constant<Type>(v), [&] {
                       if constexpr (std::same_as<Type, bool>) {
                         return v ? "true" : "false";
                       } else {
                         return std::to_string(v);
                       }
                     }()};
}

template<typename ToType, typename FromValue> inline auto bit_cast(FromValue v) {
  return detail::bit_cast_impl<FromValue, ToType>(v);
}

template<typename ToType, typename FromValue> inline auto cast(FromValue v) {
  return detail::cast_impl<FromValue, ToType>(v);
}

template<typename FunctionType, typename FunctionBuilder>
auto module_builder::create_function(std::string const& name, FunctionBuilder&& fb) {
  assert(module_builder::current_builder() == this || !module_builder::current_builder());
  exited_block_ = false;
  auto fn_ref = detail::function_builder<FunctionType>{}(name, fb);
  fn_ref.set_function_attribute({"target-cpu", llvm::sys::getHostCPUName()});
  return fn_ref;
}

auto module_builder::begin_creating_function(std::string const& name, llvm::FunctionType* func_type) {

}

auto module_builder::end_creating_function() {

}

template<typename FunctionType>
auto module_builder::declare_external_function(std::string const& name, FunctionType* fn) {
  assert(module_builder::current_builder() == this || !module_builder::current_builder());

  auto fn_ref = detail::function_declaration_builder<FunctionType>{}(name);

  declare_external_symbol(name, reinterpret_cast<void*>(fn));

  return fn_ref;
}

template<typename ReturnType, typename... Arguments>
llvm::DISubprogram* module_builder::source_code_generator::enter_function_scope(std::string const& function_name) {
  std::vector<llvm::Metadata*> dbg_types = {detail::type<ReturnType>::dbg(), detail::type<Arguments>::dbg()...};
  auto dbg_fn_type = dbg_builder_.createSubroutineType(dbg_builder_.getOrCreateTypeArray(dbg_types));
  auto dbg_fn_scope = dbg_builder_.createFunction(
      debug_scope(), function_name, function_name, debug_file(), current_line(), dbg_fn_type, current_line(),
      llvm::DINode::FlagPrototyped,
      llvm::DISubprogram::DISPFlags::SPFlagDefinition | llvm::DISubprogram::DISPFlags::SPFlagOptimized);
  dbg_scopes_.push(dbg_fn_scope);
  return dbg_fn_scope;
}

llvm::DISubprogram*
module_builder::source_code_generator::jit_enter_function_scope(std::string const& function_name, llvm::FunctionType* func_type) {
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
