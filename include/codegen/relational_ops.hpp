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
  sge,
  uge,
  sgt,
  ugt,
  sle,
  ule,
  slt,
  ult,
};

template<relational_operation_type Op> class relational_operation {
  value lhs_;
  value rhs_;

public:
  relational_operation(value lhs, value rhs) : lhs_(std::move(lhs)), rhs_(std::move(rhs)) {
    assert(lhs_.get_type() == rhs_.get_type());
    // TODO: check relational types.
  }

  value gen_value() {
    auto *val = eval();
    return value{val, fmt::format("{}", *this)};
  }

  llvm::Value* eval() const {
    if (lhs_.isIntegerType()) {
      switch (Op) {
      case relational_operation_type::eq:
        return jit_module_builder::current_builder()->ir_builder().CreateICmpEQ(lhs_.eval(), rhs_.eval());
      case relational_operation_type::ne:
        return jit_module_builder::current_builder()->ir_builder().CreateICmpNE(lhs_.eval(), rhs_.eval());
      case relational_operation_type::sge:
          return jit_module_builder::current_builder()->ir_builder().CreateICmpSGE(lhs_.eval(), rhs_.eval());
      case relational_operation_type::uge:
          return jit_module_builder::current_builder()->ir_builder().CreateICmpUGE(lhs_.eval(), rhs_.eval());
      case relational_operation_type::sgt:
          return jit_module_builder::current_builder()->ir_builder().CreateICmpSGT(lhs_.eval(), rhs_.eval());
      case relational_operation_type::ugt:
          return jit_module_builder::current_builder()->ir_builder().CreateICmpUGT(lhs_.eval(), rhs_.eval());
      case relational_operation_type::sle:
          return jit_module_builder::current_builder()->ir_builder().CreateICmpSLE(lhs_.eval(), rhs_.eval());
      case relational_operation_type::ule:
          return jit_module_builder::current_builder()->ir_builder().CreateICmpULE(lhs_.eval(), rhs_.eval());
      case relational_operation_type::slt:
          return jit_module_builder::current_builder()->ir_builder().CreateICmpSLT(lhs_.eval(), rhs_.eval());
      case relational_operation_type::ult:
          return jit_module_builder::current_builder()->ir_builder().CreateICmpULT(lhs_.eval(), rhs_.eval());
      }
    } else if (lhs_.isFloatType()) {
      switch (Op) {
      case relational_operation_type::eq:
        return jit_module_builder::current_builder()->ir_builder().CreateFCmpOEQ(lhs_.eval(), rhs_.eval());
      case relational_operation_type::ne:
        return jit_module_builder::current_builder()->ir_builder().CreateFCmpONE(lhs_.eval(), rhs_.eval());
      case relational_operation_type::sge:
        return jit_module_builder::current_builder()->ir_builder().CreateFCmpOGE(lhs_.eval(), rhs_.eval());
      case relational_operation_type::sgt:
        return jit_module_builder::current_builder()->ir_builder().CreateFCmpOGT(lhs_.eval(), rhs_.eval());
      case relational_operation_type::sle:
        return jit_module_builder::current_builder()->ir_builder().CreateFCmpOLE(lhs_.eval(), rhs_.eval());
      case relational_operation_type::slt:
        return jit_module_builder::current_builder()->ir_builder().CreateFCmpOLT(lhs_.eval(), rhs_.eval());
      default:
        llvm_unreachable("unsupported pointer arithmetic");
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
      case relational_operation_type::sge: return "(s)>=";
      case relational_operation_type::uge: return "(u)>=";
      case relational_operation_type::sgt: return "(s)>";
      case relational_operation_type::ugt: return "(u)>";
      case relational_operation_type::sle: return "(s)<=";
      case relational_operation_type::ule: return "(u)<=";
      case relational_operation_type::slt: return "(s)<";
      case relational_operation_type::ult: return "(u)<";
      }
    }();
    return os << '(' << ro.lhs_ << ' ' << symbol << ' ' << ro.rhs_ << ')';
  }
};

} // namespace detail

// TODO: support unsigned operations.
value operator==(value lhs, value rhs) {
  return detail::relational_operation<detail::relational_operation_type::eq>(std::move(lhs), std::move(rhs)).gen_value();
}

value operator!=(value lhs, value rhs) {
  return detail::relational_operation<detail::relational_operation_type::ne>(std::move(lhs), std::move(rhs)).gen_value();
}

value operator>=(value lhs, value rhs) {
  return detail::relational_operation<detail::relational_operation_type::sge>(std::move(lhs), std::move(rhs)).gen_value();
}

value operator>(value lhs, value rhs) {
  return detail::relational_operation<detail::relational_operation_type::sgt>(std::move(lhs), std::move(rhs)).gen_value();
}

value operator<=(value lhs, value rhs) {
  return detail::relational_operation<detail::relational_operation_type::sle>(std::move(lhs), std::move(rhs)).gen_value();
}

value operator<(value lhs, value rhs) {
  return detail::relational_operation<detail::relational_operation_type::slt>(std::move(lhs), std::move(rhs)).gen_value();
}

} // namespace codegen
