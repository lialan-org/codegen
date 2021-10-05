#pragma once

namespace codegen::detail {

// helper function
// https://stackoverflow.com/questions/44012938/how-to-tell-if-template-type-is-an-instance-of-a-template-class
template<class, template<class> class> struct is_instance : public std::false_type {};

template<class T, template<class> class U> struct is_instance<U<T>, U> : public std::true_type {};

// `typename` here could be change to LLVMType but that would cause clang to complain because LLVMType is
// more specialized.

// we need to carry extra type info because intermediary values
// carries a type too.
enum class runtime_type {
  BoolTy,
  IntTy,
  FloatTy,
  // TODO: more types
}; 

// an easy lookup table for JIT debugging.
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
      return codegen::jit_module_builder::current_builder()->debug_builder().createBasicType(ty_name, 8,
                                                                                         llvm::dwarf::DW_ATE_boolean);
    } else if (type->isIntegerTy()) {
      assert(!type->isIntegerTy(1));
      // TODO: implement unsigned
      return codegen::jit_module_builder::current_builder()->debug_builder().createBasicType(
        ty_name, type->getIntegerBitWidth(), llvm::dwarf::DW_ATE_signed);

    } else if (type->isFloatTy()) {
      return codegen::jit_module_builder::current_builder()->debug_builder().createBasicType(ty_name, 32,
                                                                                         llvm::dwarf::DW_ATE_float);
    } else {
      llvm_unreachable("unimplemented");
    }
  }

};

} // namespace codegen::detail
