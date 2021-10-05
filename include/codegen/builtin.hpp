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

#include "types.hpp"

namespace codegen::builtin {

using namespace codegen;

void memcpy(value dst, value src, value n) {
  using namespace detail;
  auto& mb = *jit_module_builder::current_builder();

  auto line_no = mb.source_code_.add_line(fmt::format("memcpy({}, {}, {});", dst, src, n));
  mb.ir_builder().SetCurrentDebugLocation(mb.get_debug_location(line_no, 1));
  mb.ir_builder().CreateMemCpy(
      dst.eval(), llvm::MaybeAlign(), src.eval(),
      llvm::MaybeAlign(), n.eval());
}

value<int> memcmp(Pointer auto src1, Pointer auto src2, Size auto n) {
  using namespace detail;
  auto& mb = *module_builder::current_builder();

  auto fn_type = llvm::FunctionType::get(type<int>::llvm(),
                                         {type<void*>::llvm(), type<void*>::llvm(), type<size_t>::llvm()}, false);
  auto fn = llvm::Function::Create(fn_type, llvm::GlobalValue::LinkageTypes::ExternalLinkage, "memcmp", mb.module());

  auto line_no = mb.source_code_.add_line(fmt::format("memcmp_ret = memcmp({}, {}, {});", src1, src2, n));
  mb.ir_builder().SetCurrentDebugLocation(mb.get_debug_location(line_no, 1));
  return value<int>{mb.ir_builder().CreateCall(fn, {src1.eval(), src2.eval(), n.eval()}), "memcmp_ret"};
}

namespace detail {

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

} // namespace codegen::builtin
