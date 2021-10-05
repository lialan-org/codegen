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

namespace codegen {

namespace detail {

enum class arithmetic_operation_type {
  add,
  sub,
  mul,
  div,
  mod,
  and_,
  or_,
  xor_,
};

template<arithmetic_operation_type Op> class arithmetic_operation {
  value lhs_;
  value rhs_;


public:
  using value_type = typename LHS::value_type;

  arithmetic_operation(value lhs, value rhs) : lhs_(std::move(lhs)), rhs_(std::move(rhs)) {
    assert(lhs_.getType() == rhs_.getType());
  }

  llvm::Value* eval() const {
    if constexpr (std::is_integral_v<value_type>) {
      switch (Op) {
      case arithmetic_operation_type::add:
        return codegen::module_builder::current_builder()->ir_builder().CreateAdd(lhs_.eval(), rhs_.eval());
      case arithmetic_operation_type::sub:
        return codegen::module_builder::current_builder()->ir_builder().CreateSub(lhs_.eval(), rhs_.eval());
      case arithmetic_operation_type::mul:
        return codegen::module_builder::current_builder()->ir_builder().CreateMul(lhs_.eval(), rhs_.eval());
      case arithmetic_operation_type::div:
        if constexpr (std::is_signed_v<value_type>) {
          return codegen::module_builder::current_builder()->ir_builder().CreateSDiv(lhs_.eval(), rhs_.eval());
        } else {
          return codegen::module_builder::current_builder()->ir_builder().CreateUDiv(lhs_.eval(), rhs_.eval());
        }
      case arithmetic_operation_type::mod:
        if constexpr (std::is_signed_v<value_type>) {
          return codegen::module_builder::current_builder()->ir_builder().CreateSRem(lhs_.eval(), rhs_.eval());
        } else {
          return codegen::module_builder::current_builder()->ir_builder().CreateURem(lhs_.eval(), rhs_.eval());
        }
      case arithmetic_operation_type::and_:
        return codegen::module_builder::current_builder()->ir_builder().CreateAnd(lhs_.eval(), rhs_.eval());
      case arithmetic_operation_type::or_:
        return codegen::module_builder::current_builder()->ir_builder().CreateOr(lhs_.eval(), rhs_.eval());
      case arithmetic_operation_type::xor_:
        return codegen::module_builder::current_builder()->ir_builder().CreateXor(lhs_.eval(), rhs_.eval());
      }
    } else {
      switch (Op) {
      case arithmetic_operation_type::add:
        return codegen::module_builder::current_builder()->ir_builder().CreateFAdd(lhs_.eval(), rhs_.eval());
      case arithmetic_operation_type::sub:
        return codegen::module_builder::current_builder()->ir_builder().CreateFSub(lhs_.eval(), rhs_.eval());
      case arithmetic_operation_type::mul:
        return codegen::module_builder::current_builder()->ir_builder().CreateFMul(lhs_.eval(), rhs_.eval());
      case arithmetic_operation_type::div:
        return codegen::module_builder::current_builder()->ir_builder().CreateFDiv(lhs_.eval(), rhs_.eval());
      case arithmetic_operation_type::mod:
        return codegen::module_builder::current_builder()->ir_builder().CreateFRem(lhs_.eval(), rhs_.eval());
      case arithmetic_operation_type::and_: [[fallthrough]];
      case arithmetic_operation_type::or_: [[fallthrough]];
      case arithmetic_operation_type::xor_: abort();
      }
    }
  }

  friend std::ostream& operator<<(std::ostream& os, arithmetic_operation const& ao) {
    auto symbol = [] {
      switch (Op) {
      case arithmetic_operation_type::add: return '+';
      case arithmetic_operation_type::sub: return '-';
      case arithmetic_operation_type::mul: return '*';
      case arithmetic_operation_type::div: return '/';
      case arithmetic_operation_type::mod: return '%';
      case arithmetic_operation_type::and_: return '&';
      case arithmetic_operation_type::or_: return '|';
      case arithmetic_operation_type::xor_: return '^';
      }
    }();
    return os << '(' << ao.lhs_ << ' ' << symbol << ' ' << ao.rhs_ << ')';
  }
};

enum class pointer_arithmetic_operation_type {
  add,
  sub,
};

template<pointer_arithmetic_operation_type Op> class pointer_arithmetic_operation {
  LHS lhs_;
  RHS rhs_;

public:
  pointer_arithmetic_operation(LHS lhs, RHS rhs) : lhs_(std::move(lhs)), rhs_(std::move(rhs)) {
    assert(lhs_.getType().isPointerTy());
    assert(rhs_.getType().isFirstClassType());
  }

  llvm::Value* eval() const {
    auto& mb = *codegen::module_builder::current_builder();
    auto rhs = rhs_.eval();
    llvm_unreachable("unimplemented");
    /*
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

  friend std::ostream& operator<<(std::ostream& os, pointer_arithmetic_operation const& ao) {
    auto symbol = [] {
      switch (Op) {
      case pointer_arithmetic_operation_type::add: return '+';
      case pointer_arithmetic_operation_type::sub: return '-';
      }
    }();
    return os << '(' << ao.lhs_ << ' ' << symbol << ' ' << ao.rhs_ << ')';
  }
};

} // namespace detail

auto operator+(value lhs, value rhs) {
  return detail::arithmetic_operation<detail::arithmetic_operation_type::add>(std::move(lhs), std::move(rhs));
}

auto operator-(value lhs, value rhs) {
  return detail::arithmetic_operation<detail::arithmetic_operation_type::sub>(std::move(lhs), std::move(rhs));
}

auto operator*(value lhs, value rhs) {
  return detail::arithmetic_operation<detail::arithmetic_operation_type::mul>(std::move(lhs), std::move(rhs));
}

auto operator/(value lhs, value rhs) {
  return detail::arithmetic_operation<detail::arithmetic_operation_type::div>(std::move(lhs), std::move(rhs));
}

auto operator%(value lhs, value rhs) {
  return detail::arithmetic_operation<detail::arithmetic_operation_type::mod>(std::move(lhs), std::move(rhs));
}

auto operator&(value lhs, value rhs) {
  return detail::arithmetic_operation<detail::arithmetic_operation_type::and_>(std::move(lhs), std::move(rhs));
}

auto operator|(value lhs, value rhs) {
  return detail::arithmetic_operation<detail::arithmetic_operation_type::or_>(std::move(lhs), std::move(rhs));
}

auto operator^(value lhs, value rhs) {
  return detail::arithmetic_operation<detail::arithmetic_operation_type::xor_>(std::move(lhs), std::move(rhs));
}

/*
template<PointerValue LHS, IntegralValue RHS> auto operator+(LHS lhs, RHS rhs) {
  return detail::pointer_arithmetic_operation<detail::pointer_arithmetic_operation_type::add, LHS, RHS>(std::move(lhs),
                                                                                                        std::move(rhs));
}

template<PointerValue LHS, IntegralValue RHS> auto operator-(LHS lhs, RHS rhs) {
  return detail::pointer_arithmetic_operation<detail::pointer_arithmetic_operation_type::sub, LHS, RHS>(std::move(lhs),
                                                                                                        std::move(rhs));
}
*/

} // namespace codegen
