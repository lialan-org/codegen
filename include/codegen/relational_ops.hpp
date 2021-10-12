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

class relational_operations {
  relational_operation_type op_;
  value lhs_;
  value rhs_;

public:
  relational_operations(relational_operation_type op, value lhs, value rhs)
      : op_(op), lhs_(std::move(lhs)), rhs_(std::move(rhs)) {
    assert(lhs_.get_type() == rhs_.get_type());
    // TODO: check relational types.
  }

  value gen_value() {
    auto *val = eval();
    return value{val, fmt::format("{}", *this)};
  }

  llvm::Value* eval() const;

  friend std::ostream& operator<<(std::ostream& os, relational_operations const& ro);
};

} // namespace detail

// TODO: support unsigned operations.
inline value operator==(value lhs, value rhs) {
  return detail::relational_operations(detail::relational_operation_type::eq, std::move(lhs), std::move(rhs)).gen_value();
}

inline value operator!=(value lhs, value rhs) {
  return detail::relational_operations(detail::relational_operation_type::ne, std::move(lhs), std::move(rhs)).gen_value();
}

inline value operator>=(value lhs, value rhs) {
  return detail::relational_operations(detail::relational_operation_type::sge, std::move(lhs), std::move(rhs)).gen_value();
}

inline value operator>(value lhs, value rhs) {
  return detail::relational_operations(detail::relational_operation_type::sgt, std::move(lhs), std::move(rhs)).gen_value();
}

inline value operator<=(value lhs, value rhs) {
  return detail::relational_operations(detail::relational_operation_type::sle, std::move(lhs), std::move(rhs)).gen_value();
}

inline value operator<(value lhs, value rhs) {
  return detail::relational_operations(detail::relational_operation_type::slt, std::move(lhs), std::move(rhs)).gen_value();
}

} // namespace codegen
