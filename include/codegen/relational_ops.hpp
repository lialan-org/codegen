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

namespace codegen {

namespace detail {

enum class relational_operation_type {
  eq,
  ne,
  ge,
  gt,
  le,
  lt,
};

template<relational_operation_type Op> class relational_operation {
  value lhs_;
  value rhs_;

public:
  using value_type = bool;

  relational_operation(value lhs, value rhs) : lhs_(std::move(lhs)), rhs_(std::move(rhs)) {
    assert(lhs_.getType() == rhs_.getType());
  }

  llvm::Value* eval() const {
    if (lhs_.getType().isIntegerTy()) {
      switch (Op) {
      case relational_operation_type::eq:
        return codegen::module_builder::current_builder()->ir_builder().CreateICmpEQ(lhs_.eval(), rhs_.eval());
      case relational_operation_type::ne:
        return codegen::module_builder::current_builder()->ir_builder().CreateICmpNE(lhs_.eval(), rhs_.eval());
      case relational_operation_type::ge:
        if constexpr (std::is_signed_v<operand_type>) {
          return codegen::module_builder::current_builder()->ir_builder().CreateICmpSGE(lhs_.eval(), rhs_.eval());
        } else {
          return codegen::module_builder::current_builder()->ir_builder().CreateICmpUGE(lhs_.eval(), rhs_.eval());
        }
      case relational_operation_type::gt:
        if constexpr (std::is_signed_v<operand_type>) {
          return codegen::module_builder::current_builder()->ir_builder().CreateICmpSGT(lhs_.eval(), rhs_.eval());
        } else {
          return codegen::module_builder::current_builder()->ir_builder().CreateICmpUGT(lhs_.eval(), rhs_.eval());
        }
      case relational_operation_type::le:
        if constexpr (std::is_signed_v<operand_type>) {
          return codegen::module_builder::current_builder()->ir_builder().CreateICmpSLE(lhs_.eval(), rhs_.eval());
        } else {
          return codegen::module_builder::current_builder()->ir_builder().CreateICmpULE(lhs_.eval(), rhs_.eval());
        }
      case relational_operation_type::lt:
        if constexpr (std::is_signed_v<operand_type>) {
          return codegen::module_builder::current_builder()->ir_builder().CreateICmpSLT(lhs_.eval(), rhs_.eval());
        } else {
          return codegen::module_builder::current_builder()->ir_builder().CreateICmpULT(lhs_.eval(), rhs_.eval());
        }
      }
    } else if (lhs_.getType().isFloatTy()) {
      switch (Op) {
      case relational_operation_type::eq:
        return codegen::module_builder::current_builder()->ir_builder().CreateFCmpOEQ(lhs_.eval(), rhs_.eval());
      case relational_operation_type::ne:
        return codegen::module_builder::current_builder()->ir_builder().CreateFCmpONE(lhs_.eval(), rhs_.eval());
      case relational_operation_type::ge:
        return codegen::module_builder::current_builder()->ir_builder().CreateFCmpOGE(lhs_.eval(), rhs_.eval());
      case relational_operation_type::gt:
        return codegen::module_builder::current_builder()->ir_builder().CreateFCmpOGT(lhs_.eval(), rhs_.eval());
      case relational_operation_type::le:
        return codegen::module_builder::current_builder()->ir_builder().CreateFCmpOLE(lhs_.eval(), rhs_.eval());
      case relational_operation_type::lt:
        return codegen::module_builder::current_builder()->ir_builder().CreateFCmpOLT(lhs_.eval(), rhs_.eval());
      }
    } else {
      llvm_unreachable("unimplemented");
    }
  }

  friend std::ostream& operator<<(std::ostream& os, relational_operation const& ro) {
    auto symbol = [] {
      switch (Op) {
      case relational_operation_type::eq: return "==";
      case relational_operation_type::ne: return "!=";
      case relational_operation_type::ge: return ">=";
      case relational_operation_type::gt: return ">";
      case relational_operation_type::le: return "<=";
      case relational_operation_type::lt: return "<";
      }
    }();
    return os << '(' << ro.lhs_ << ' ' << symbol << ' ' << ro.rhs_ << ')';
  }
};

} // namespace detail

auto operator==(value lhs, value rhs) {
  return detail::relational_operation<detail::relational_operation_type::eq>(std::move(lhs), std::move(rhs));
}

auto operator!=(value lhs, value rhs) {
  return detail::relational_operation<detail::relational_operation_type::ne>(std::move(lhs), std::move(rhs));
}

auto operator>=(value lhs, value rhs) {
  return detail::relational_operation<detail::relational_operation_type::ge>(std::move(lhs), std::move(rhs));
}

auto operator>(value lhs, value rhs) {
  return detail::relational_operation<detail::relational_operation_type::gt>(std::move(lhs), std::move(rhs));
}

auto operator<=(value lhs, value rhs) {
  return detail::relational_operation<detail::relational_operation_type::le>(std::move(lhs), std::move(rhs));
}

auto operator<(value lhs, value rhs) {
  return detail::relational_operation<detail::relational_operation_type::lt>(std::move(lhs), std::move(rhs));
}

} // namespace codegen
