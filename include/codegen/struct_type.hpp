
#include <vector>


namespace codegen::detail {
  template<typename FirstType, typename ... OtherTypes>
  void getTypeArray(std::vector<int> &type_vec) {

  }


  template<typename Type>
  void getTypeArray(std::vector<int> &type_vec) {

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
  //static constexpr std::size_t size = sizeof...(Args);

  Struct() {}

  static void declare_type() {
    std::vector<int> type_vec;
    ::codegen::detail::getTypeArray<Args...>(type_vec);
  }

  //template<typename Y>
  //value<Y> field(TupleType t) {

  //}

  // declare()
  // 
  // llvm()
  //
  // dbg()
private:
};

} // namespace codegen
