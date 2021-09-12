#pragma once

namespace codegen::detail {

// helper function
// https://stackoverflow.com/questions/44012938/how-to-tell-if-template-type-is-an-instance-of-a-template-class
template <class, template <class> class>
struct is_instance : public std::false_type {};

template <class T, template <class> class U>
struct is_instance<U<T>, U> : public std::true_type {};

// `typename` here could be change to LLVMType but that would cause clang to complain because LLVMType is
// more specialized.

template<typename Type>
struct type {
  static constexpr size_t alignment = alignof(Type);
  static llvm::DIType* dbg() {
    return current_builder->dbg_builder_.createBasicType(
        name(), sizeof(Type) * 8, std::is_signed_v<Type> ? llvm::dwarf::DW_ATE_signed : llvm::dwarf::DW_ATE_unsigned);
  }
  static llvm::Type* llvm() { return llvm::Type::getIntNTy(*current_builder->context_, sizeof(Type) * 8); }
  static std::string name() { return fmt::format("{}{}", std::is_signed_v<Type> ? 'i' : 'u', sizeof(Type) * 8); }
};

template<>
struct type<void> {
  static constexpr size_t alignment = 0;
  static llvm::DIType* dbg() { return nullptr; }
  static llvm::Type* llvm() { return llvm::Type::getVoidTy(*current_builder->context_); }
  static std::string name() { return "void"; }
};

template<>
struct type<bool> {
  static constexpr size_t alignment = alignof(bool);
  static llvm::DIType* dbg() {
    return current_builder->dbg_builder_.createBasicType(name(), 8, llvm::dwarf::DW_ATE_boolean);
  }
  static llvm::Type* llvm() { return llvm::Type::getInt1Ty(*current_builder->context_); }
  static std::string name() { return "bool"; }
};

template<>
struct type<std::byte> {
  static constexpr size_t alignment = 1;
  static llvm::DIType* dbg() {
    return current_builder->dbg_builder_.createBasicType(name(), 8, llvm::dwarf::DW_ATE_unsigned);
  }
  static llvm::Type* llvm() { return llvm::Type::getInt8Ty(*current_builder->context_); }
  static std::string name() { return "byte"; }
};

template<>
struct type<float> {
  static constexpr size_t alignment = alignof(float);
  static llvm::DIType* dbg() {
    return current_builder->dbg_builder_.createBasicType(name(), 32, llvm::dwarf::DW_ATE_float);
  }
  static llvm::Type* llvm() { return llvm::Type::getFloatTy(*current_builder->context_); }
  static std::string name() { return "f32"; }
};

template<>
struct type<double> {
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

} // namespace codegen::detail

