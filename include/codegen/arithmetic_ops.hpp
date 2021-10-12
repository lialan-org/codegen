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

namespace codegen {

namespace detail {

enum class arithmetic_operation_type {
  add,
  sub,
  mul,
  sdiv,
  udiv,
  smod,
  umod,
  and_,
  or_,
  xor_,
};

class arithmetic_operations {
  arithmetic_operation_type op_;
  value lhs_;
  value rhs_;

  llvm::Value* eval() const;

public:
  arithmetic_operations(arithmetic_operation_type op, value lhs, value rhs) : op_(op), lhs_(std::move(lhs)), rhs_(std::move(rhs)) {
    assert(lhs_.get_type() == rhs_.get_type());
  }

  value gen_value() {
    auto *val = eval();
    return value{val, fmt::format("{}", *this)};
  }

  friend std::ostream& operator<<(std::ostream& os, arithmetic_operations const& ao);
};

enum class pointer_arithmetic_operation_type {
  add,
  sub,
};

class pointer_arithmetic_operations {
  pointer_arithmetic_operation_type op_;
  value lhs_;
  value rhs_;

  llvm::Value* eval() const {
    llvm_unreachable("unimplemented");
    /*
    //auto& mb = *codegen::jit_module_builder::current_builder();
    //auto rhs = rhs_.eval();
    if constexpr (sizeof(rhs_value_type) < sizeof(uint64_t)) {
      if constexpr (std::is_unsigned_v<rhs_value_type>) {
        rhs = mb.ir_builder().CreateZExt(rhs, type<uint64_t>::llvm());
      } else {
        rhs = mb.ir_builder().CreateSExt(rhs, type<int64_t>::llvm());
      }
    }
    switch (Op) {
    case pointer_arithmetic_operation_type::add: return mb.ir_builder().CreateInBoundsGEP(lhs_.eval(), rhs);
    case pointer_arithmetic_operation_type::sub:
      return mb.ir_builder().CreateInBoundsGEP(lhs_.eval(), mb.ir_builder().CreateSub(constant<int64_t>(0), rhs));
    }
    abort();
    */
  }

public:
  pointer_arithmetic_operations(pointer_arithmetic_operation_type op, value lhs, value rhs)
      : op_(op), lhs_(std::move(lhs)), rhs_(std::move(rhs)) {
    assert(lhs_.isPointerType());
    // TODO: more types
    assert(rhs_.isIntegerType() || rhs_.isPointerType() || rhs_.isFloatType());
  }

  value gen_value() {
    auto *val = eval();
    return value{val, fmt::format("{}", *this)};
  }

  friend std::ostream& operator<<(std::ostream& os, pointer_arithmetic_operations const& ao);
};


} // namespace detail

// TODO: add unsigned operations.

value operator+(value lhs, value rhs) {
  if (lhs.isPointerType()) {
    return detail::pointer_arithmetic_operations(detail::pointer_arithmetic_operation_type::add, std::move(lhs), std::move(rhs)).gen_value();
  } else {
    return detail::arithmetic_operations(detail::arithmetic_operation_type::add, std::move(lhs), std::move(rhs)).gen_value();
  }
}

value operator-(value lhs, value rhs) {
  if (lhs.isPointerType()) {
    return detail::pointer_arithmetic_operations(detail::pointer_arithmetic_operation_type::sub, std::move(lhs), std::move(rhs)).gen_value();
  } else {
    return detail::arithmetic_operations(detail::arithmetic_operation_type::sub, std::move(lhs), std::move(rhs)).gen_value();
  }
}

value operator*(value lhs, value rhs) {
  return detail::arithmetic_operations(detail::arithmetic_operation_type::mul, std::move(lhs), std::move(rhs)).gen_value();
}

value operator/(value lhs, value rhs) {
  return detail::arithmetic_operations(detail::arithmetic_operation_type::sdiv, std::move(lhs), std::move(rhs)).gen_value();
}

value operator%(value lhs, value rhs) {
  return detail::arithmetic_operations(detail::arithmetic_operation_type::umod, std::move(lhs), std::move(rhs)).gen_value();
}

value operator&(value lhs, value rhs) {
  return detail::arithmetic_operations(detail::arithmetic_operation_type::and_, std::move(lhs), std::move(rhs)).gen_value();
}

value operator|(value lhs, value rhs) {
  return detail::arithmetic_operations(detail::arithmetic_operation_type::or_, std::move(lhs), std::move(rhs)).gen_value();
}

value operator^(value lhs, value rhs) {
  return detail::arithmetic_operations(detail::arithmetic_operation_type::xor_, std::move(lhs), std::move(rhs)).gen_value();
}

} // namespace codegen
