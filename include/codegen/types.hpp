#pragma once

#include "codegen/module_builder.hpp"
#include "codegen/value.hpp"

#include <sstream>

#include <concepts>

namespace codegen::detail {
// We have another flavor of wrappers inside codegen::jit namespace. 
// IT does not take template arguments because we need to:
// 1. run it at runtime 
// 2. do codegen incrementally
// 3. need to store intemediary results.
struct type_reverse_lookup {
  static std::string name(llvm::Type *type) {
    if (type->isVoidTy()) {
      return "void";
    } else if (type->isIntegerTy(1)) {
      return "bool";
    } else if (type->isIntegerTy()) {
      int32_t bitwidth = type->getIntegerBitWidth();
      return fmt::format("s{}", bitwidth);
    }else if (type->isFloatTy()) {
      return "f32";
    } else {
      llvm_unreachable("unimplemented");
    }
  }

  static llvm::DIType* dbg(llvm::Type* type)  {
    std::string ty_name = type_reverse_lookup::name(type);
    if (type->isVoidTy()) {
      return nullptr;      
    } else if (type->isIntegerTy(1)) {
      // bool type
      return jit_module_builder::current_builder()->debug_builder().createBasicType(ty_name, 8,
                                                                                         llvm::dwarf::DW_ATE_boolean);
    } else if (type->isIntegerTy()) {
      assert(!type->isIntegerTy(1));
      // TODO: implement unsigned
      return jit_module_builder::current_builder()->debug_builder().createBasicType(
        ty_name, type->getIntegerBitWidth(), llvm::dwarf::DW_ATE_signed);

    } else if (type->isFloatTy()) {
      return jit_module_builder::current_builder()->debug_builder().createBasicType(ty_name, 32,
                                                                                         llvm::dwarf::DW_ATE_float);
    } else {
      llvm_unreachable("unimplemented");
    }
  }
};

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

} // namespace codegen
