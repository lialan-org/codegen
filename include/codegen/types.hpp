#pragma once

#include "module_builder.hpp"

#include <concepts>

namespace codegen {

template<typename...>
inline constexpr bool false_v = false;

template<typename T>
concept Pointer = std::is_pointer_v<typename T::value_type>;

template<typename S>
concept Size = std::is_same_v<typename S::value_type, int32_t> ||
               std::is_same_v<typename S::value_type, int64_t>;

template<typename T>
concept Integral = std::is_integral_v<T>; // <concepts> does not have it??

template<typename T>
concept Bool = std::same_as<T, bool>;

template<typename T>
concept Void = std::same_as<T, void>;

template<typename T>
concept Byte = std::same_as<T, std::byte>;

template<typename T>
concept Float = std::same_as<T, float>;

template<typename T>
concept Double = std::same_as<T, double>;

}; // namespace codegen

namespace codegen::detail {

template<typename> struct type;

template<Integral Type>
struct type<Type> {
  static constexpr size_t alignment = alignof(Type);
  static llvm::DIType* dbg() {
    return current_builder->dbg_builder_.createBasicType(
        name(), sizeof(Type) * 8, std::is_signed_v<Type> ? llvm::dwarf::DW_ATE_signed : llvm::dwarf::DW_ATE_unsigned);
  }
  static llvm::Type* llvm() { return llvm::Type::getIntNTy(*current_builder->context_, sizeof(Type) * 8); }
  static std::string name() { return fmt::format("{}{}", std::is_signed_v<Type> ? 'i' : 'u', sizeof(Type) * 8); }
};

template<Void Type>
struct type<Type> {
  static constexpr size_t alignment = 0;
  static llvm::DIType* dbg() { return nullptr; }
  static llvm::Type* llvm() { return llvm::Type::getVoidTy(*current_builder->context_); }
  static std::string name() { return "void"; }
};

template<Bool Type>
struct type<Type> {
  static constexpr size_t alignment = alignof(bool);
  static llvm::DIType* dbg() {
    return current_builder->dbg_builder_.createBasicType(name(), 8, llvm::dwarf::DW_ATE_boolean);
  }
  static llvm::Type* llvm() { return llvm::Type::getInt1Ty(*current_builder->context_); }
  static std::string name() { return "bool"; }
};

template<Byte Type>
struct type<Type> {
  static constexpr size_t alignment = 1;
  static llvm::DIType* dbg() {
    return current_builder->dbg_builder_.createBasicType(name(), 8, llvm::dwarf::DW_ATE_unsigned);
  }
  static llvm::Type* llvm() { return llvm::Type::getInt8Ty(*current_builder->context_); }
  static std::string name() { return "byte"; }
};

template<Float Type>
struct type<Type> {
  static constexpr size_t alignment = alignof(float);
  static llvm::DIType* dbg() {
    return current_builder->dbg_builder_.createBasicType(name(), 32, llvm::dwarf::DW_ATE_float);
  }
  static llvm::Type* llvm() { return llvm::Type::getFloatTy(*current_builder->context_); }
  static std::string name() { return "f32"; }
};

template<Double Type>
struct type<Type> {
  static constexpr size_t alignment = alignof(double);
  static llvm::DIType* dbg() {
    return current_builder->dbg_builder_.createBasicType(name(), 64, llvm::dwarf::DW_ATE_float);
  }
  static llvm::Type* llvm() { return llvm::Type::getDoubleTy(*current_builder->context_); }
  static std::string name() { return "f64"; }
};

template<typename Type>
struct type<Type*> {
  static constexpr size_t alignment = alignof(Type*);
  static llvm::DIType* dbg() {
    return current_builder->dbg_builder_.createPointerType(type<std::remove_cv_t<Type>>::dbg(), sizeof(Type*) * 8);
  }
  static llvm::Type* llvm() { return type<std::remove_cv_t<Type>>::llvm()->getPointerTo(); }
  static std::string name() { return type<std::remove_cv_t<Type>>::name() + '*'; }
};

// array type
template<typename Type, size_t N>
struct type<Type[N]> {
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

}; // namespace codegen::namespace

namespace codegen {

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

template<typename ToType, typename FromValue> auto bit_cast(FromValue v) {
  return detail::bit_cast_impl<FromValue, ToType>(v);
}

template<typename ToType, typename FromValue> auto cast(FromValue v) {
  return detail::cast_impl<FromValue, ToType>(v);
}

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

template<typename FunctionType>
auto module_builder::declare_external_function(std::string const& name, FunctionType* fn) {
  assert(detail::current_builder == this || !detail::current_builder);

  auto prev_builder = std::exchange(detail::current_builder, this);
  auto fn_ref = detail::function_declaration_builder<FunctionType>{}(name);
  detail::current_builder = prev_builder;

  declare_external_symbol(name, reinterpret_cast<void*>(fn));

  return fn_ref;
}

}; // namespace codegen

