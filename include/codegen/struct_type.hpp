
#include <tuple>

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
  using TupleType = std::tuple<Args...>;
  //static constexpr std::size_t size = sizeof...(Args);

  Struct(Args... values) {
    fields_ = std::tuple<Args...>(values...);
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
  TupleType fields_;
};

} // namespace codegen
