/*
 * Copyright © 2019 Paweł Dziepak
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include "codegen/module_builder.hpp"
#include "codegen/types.hpp"

namespace codegen::builtin {

using namespace codegen;

[[maybe_unused]]
inline static void register_printf(llvm::LLVMContext &context, llvm::Module &module) {
    using namespace llvm;
    std::vector<Type*> args;
    args.push_back (Type::getInt8PtrTy (context));
    /*`true` specifies the function as variadic*/
    FunctionType* printfType = FunctionType::get (llvm::Type::getInt32Ty (context), args, true);
    Function::Create (printfType, Function::ExternalLinkage, "printf", module);
}

template<typename T>
inline value bitcast(value src) {
  llvm::Type* target_type = detail::type_reverse_lookup::type<T>();
  llvm::Type* original_type = src.get_type();

  if (target_type == original_type) {
    return src;
  }

  auto& mb = *jit_module_builder::current_builder();
  auto casted_value = mb.ir_builder().CreateBitCast(src.eval(), target_type);
  return value{casted_value, src.get_name() + "_casted"}; 
}

inline void memcpy(value dst, value src, value n) {
  assert(n.isIntegerType());

  auto& mb = *jit_module_builder::current_builder();

  auto line_no = mb.source_code_.add_line(fmt::format("memcpy({}, {}, {});", dst, src, n));
  mb.ir_builder().SetCurrentDebugLocation(mb.get_debug_location(line_no, 1));
  mb.ir_builder().CreateMemCpy(
      dst.eval(), llvm::MaybeAlign(), src.eval(),
      llvm::MaybeAlign(), n.eval());
}

inline value memcmp(value src1, value src2, value n) {
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
