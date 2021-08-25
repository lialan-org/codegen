#include <vector>

namespace codegen::detail {

  template<typename Type>
  void getTypeArrayEnd(std::vector<int> &type_vec) {
    printf("single typename get\n");
    type_vec.push_back(0);
  }

  template<typename FirstType, typename ... OtherTypes>
  void getTypeArray(std::vector<int> &type_vec) {
    printf("multiple typename get\n");
    type_vec.push_back(1);
    if constexpr (sizeof...(OtherTypes) == 1) {
      getTypeArrayEnd<OtherTypes...>(type_vec);
    } else {
      getTypeArray<OtherTypes...>(type_vec);
    }
  }
}

namespace codegen {

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

  static void declare_type() {
    std::vector<int> type_vec;
    ::codegen::detail::getTypeArray<Args...>(type_vec);

    printf("arg size: %lu\n", type_vec.size());
  }

  // declare()
  // 
  // llvm()
  //
  // dbg()
private:
};

} // namespace codegen

/*
int main() {
  codegen::Struct<int, int, float>::declare_type();
  codegen::Struct<float, int>::declare_type();
}
*/
