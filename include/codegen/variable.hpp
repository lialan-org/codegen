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

#include "module_builder.hpp"
#include "types.hpp"

namespace codegen {

class variable : public value {
  explicit variable(std::string const& n, llvm::Type *llvm_type);

public:
  template<int size>
  static variable variable_integer(std::string const &n) {
    auto& mb = *jit_module_builder::current_builder();
    auto& context = mb.context();
    auto llvm_type = llvm::IntegerType::get(context, size);
    return variable(n, llvm_type);
  }

  void set(value const& v);

  value get() const;

  static variable boolean(std::string const& n) {
    return variable_integer<1>(n);
  }

  static variable i32(std::string const &n) {
    return variable_integer<32>(n);
  }

  static variable i64(std::string const &n) {
    return variable_integer<64>(n);
  }

  static variable f32(std::string const &n) {
    auto& mb = *jit_module_builder::current_builder();
    auto& context = mb.context();
    llvm::Type *llvm_type = llvm::Type::getFloatTy(context);
    return variable(n, llvm_type);
  }

  // TODO: array, struct, double, string, byte types.
}; // class variable

} // namespace codegen
