#include "codegen/builtin.hpp"

namespace codegen::builtin {

using namespace codegen;

[[maybe_unused]]
void register_printf(llvm::LLVMContext &context, llvm::Module &module) {
    using namespace llvm;
    std::vector<Type*> args;
    args.push_back (Type::getInt8PtrTy (context));
    /*`true` specifies the function as variadic*/
    FunctionType* printfType = FunctionType::get (llvm::Type::getInt32Ty (context), args, true);
    Function::Create (printfType, Function::ExternalLinkage, "printf", module);
}

value gep(value src, size_t index) {
  using namespace llvm;
  assert(src.get_type()->isPointerTy());
  Type* elem_type = src.get_type()->getPointerElementType();

  auto &mb = *jit_module_builder::current_builder();
  auto &builder = mb.ir_builder();
  auto &context = builder.getContext();

  auto c_index = ConstantInt::get(context, llvm::APInt(64, index));

  auto * gep_inst = GetElementPtrInst::CreateInBounds(src, {c_index});

  return value{gep_inst, src.get_name() + "_" + std::to_string(index)};
}

void memcpy(value dst, value src, value n) {
  assert(n.isIntegerType());

  auto& mb = *jit_module_builder::current_builder();

  auto line_no = mb.source_code_.add_line(fmt::format("memcpy({}, {}, {});", dst, src, n));
  mb.ir_builder().SetCurrentDebugLocation(mb.get_debug_location(line_no, 1));
  mb.ir_builder().CreateMemCpy(
      dst.eval(), llvm::MaybeAlign(), src.eval(),
      llvm::MaybeAlign(), n.eval());
}

value memcmp(value src1, value src2, value n) {
  assert(src1.isPointerType() && src2.isPointerType());
  auto& mb = *jit_module_builder::current_builder();

  auto *i32type = llvm::IntegerType::get(mb.context(), 32);
  auto *void_star_type = llvm::PointerType::getUnqual(llvm::Type::getVoidTy(mb.context()));

  auto size_t_type = llvm::IntegerType::get(mb.context(), 64);

  auto fn_type = llvm::FunctionType::get(i32type,
                                         {void_star_type, void_star_type, size_t_type}, false);
  auto fn = llvm::Function::Create(fn_type, llvm::GlobalValue::LinkageTypes::ExternalLinkage, "memcmp", mb.module());

  auto line_no = mb.source_code_.add_line(fmt::format("memcmp_ret = memcmp({}, {}, {});", src1, src2, n));
  mb.ir_builder().SetCurrentDebugLocation(mb.get_debug_location(line_no, 1));
  return value{mb.ir_builder().CreateCall(fn, {src1.eval(), src2.eval(), n.eval()}), "memcmp_ret"};
}

namespace detail {

/*
template<typename Value> class bswap_impl {
  Value value_;

public:
  using value_type = typename Value::value_type;
  static_assert(std::is_integral_v<value_type>);

  explicit bswap_impl(Value v) : value_(v) {}

  llvm::Value* eval() {
    return codegen::module_builder::current_builder()->ir_builder().CreateUnaryIntrinsic(llvm::Intrinsic::bswap,
                                                                                         value_.eval());
  }

  friend std::ostream& operator<<(std::ostream& os, bswap_impl bi) { return os << "bswap(" << bi.value_ << ")"; }
};

} // namespace detail

template<typename Value> auto bswap(Value v) {
  return detail::bswap_impl<Value>(v);
}
*/

} //namespace detail

} // namespace codegen::builtin
