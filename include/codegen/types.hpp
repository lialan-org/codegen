#pragma once

#include "codegen/module_builder.hpp"
#include "codegen/value.hpp"
#include "codegen/types.hpp"

#include <sstream>

namespace codegen::detail {
// We have another flavor of wrappers inside codegen::jit namespace. 
// IT does not take template arguments because we need to:
// 1. run it at runtime 
// 2. do codegen incrementally
// 3. need to store intemediary results.
struct type_reverse_lookup {
  template<typename T>
  static inline llvm::Type* type() {
    using namespace llvm;
    
    auto &builder = jit_module_builder::current_builder()->ir_builder();
    auto &context = builder.getContext();

    // TODO: better utilize pointer types.
    if constexpr (std::is_same_v<T, bool>) {
      return builder.getInt1Ty();
    } else if constexpr (std::is_same_v<T, int32_t>) {
      return builder.getInt32Ty();
    } else if constexpr (std::is_same_v<T, int64_t>) {
      return builder.getInt64Ty();
    } else if constexpr (std::is_same_v<T, float>) {
      return builder.getFloatTy();
    } else if constexpr (std::is_same_v<T, bool*>) {
      return PointerType::getInt1PtrTy(context);
    } else if constexpr (std::is_same_v<T, int32_t*>) {
      return PointerType::getInt32PtrTy(context);
    } else if constexpr (std::is_same_v<T, int64_t*>) {
      return PointerType::getInt64PtrTy(context);
    } else if constexpr (std::is_same_v<T, float*>) {
      return PointerType::getFloatPtrTy(context);
    } else {
      llvm_unreachable("unimplemented");
    }

  }

  static inline std::string name(llvm::Type *type) {
    if (type->isVoidTy()) {
      return "void";
    } else if (type->isIntegerTy(1)) {
      return "bool";
    } else if (type->isIntegerTy()) {
      int32_t bitwidth = type->getIntegerBitWidth();
      return fmt::format("s{}", bitwidth);
    }else if (type->isFloatTy()) {
      return "f32";
    } else if (type->isPointerTy()) {
      llvm::PointerType * ptr_type = dyn_cast<llvm::PointerType>(type);
      llvm::Type *elem_type = ptr_type->getElementType();
      return "*" + name(elem_type);
    } else {
      llvm_unreachable("unimplemented");
    }
  }

  static inline llvm::DIType* dbg(llvm::Type* type)  {
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
    } else if (type->isPointerTy()) {
      llvm::PointerType * ptr_type = dyn_cast<llvm::PointerType>(type);
      llvm::Type *elem_type = ptr_type->getElementType();
      assert(elem_type->isSized());

      return jit_module_builder::current_builder()->debug_builder().createPointerType(dbg(elem_type), getTypeSize(elem_type));
    } else {
      llvm_unreachable("unimplemented");
    }
  }

  // data size are dependent on target data layout.
  static inline llvm::TypeSize getTypeSize(llvm::Type * type) {
    auto &module =  jit_module_builder::current_builder()->module();

    if (type->isIntegerTy() || type->isFloatTy() || type->isPointerTy()) {
      return module.getDataLayout().getTypeAllocSizeInBits(type);
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
