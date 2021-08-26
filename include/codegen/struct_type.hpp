#pragma once

#include "codegen/module_builder.hpp"
#include <vector>

namespace codegen {

namespace detail {
  template<typename T>
  llvm::Type* getType() {
    return type<T>::llvm();
  }

  template<typename FirstType, typename ... OtherTypes>
  void getTypeArray(std::vector<llvm::Type*> &type_vec) {
    type_vec.push_back(getType<FirstType>());
    if constexpr (sizeof...(OtherTypes) == 1) {
      type_vec.push_back(getType<OtherTypes...>());
    } else {
      getTypeArray<OtherTypes...>(type_vec);
    }
  }
}

// Supposely, this would work like this:
// Declare:
// typedef Struct<int32_t, float*> ExampleStruct;
// builder.declare_type(ExampleStruct);
//
// Use:
// auto s = codegen::variable<ExampleStruct>("example_struct");
// s.get<int>(1).set(2_i32);


template<typename... Args>
class Struct {
public:
  static_assert(sizeof...(Args) > 0);

  Struct() {}

  static llvm::Type* llvm(std::string name = "struct_type") {
    std::vector<llvm::Type*> type_vec;
    ::codegen::detail::getTypeArray<Args...>(type_vec);

    auto& mb = *detail::current_builder;

    llvm::StructType* const struct_type = llvm::StructType::create(*mb.context_, name);
    struct_type->setBody(type_vec);
    return struct_type;
  }

  // declare()
  // 
  // llvm()
  //
  // dbg()
private:
};

namespace detail {
  template<typename... Args>
  struct type<codegen::Struct<Args...>> {
    using struct_type = codegen::Struct<Args...>;
    static constexpr size_t alignment = alignof(int);
    static llvm::DIType* dbg() {
      return current_builder->dbg_builder_.createPointerType(type<int*>::dbg(), sizeof(int*) * 8);
    }
    static llvm::Type* llvm() { return Struct<Args...>::llvm(); }
    static std::string name() { return "StructType"; }
  };
}

} // namespace codegen

/*
int main() {
  codegen::Struct<int, int, float>::declare_type();
  codegen::Struct<float, int>::declare_type();
}
*/
