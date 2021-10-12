
#include "codegen/arithmetic_ops.hpp"

namespace codegen::detail {

llvm::Value* arithmetic_operations::eval() const {
  if (lhs_.isIntegerType()) {
    switch (op_) {
    case arithmetic_operation_type::add:
      return codegen::jit_module_builder::current_builder()->ir_builder().CreateAdd(lhs_.eval(), rhs_.eval());
    case arithmetic_operation_type::sub:
      return codegen::jit_module_builder::current_builder()->ir_builder().CreateSub(lhs_.eval(), rhs_.eval());
    case arithmetic_operation_type::mul:
      return codegen::jit_module_builder::current_builder()->ir_builder().CreateMul(lhs_.eval(), rhs_.eval());
    case arithmetic_operation_type::sdiv:
        return codegen::jit_module_builder::current_builder()->ir_builder().CreateSDiv(lhs_.eval(), rhs_.eval());
    case arithmetic_operation_type::udiv:
        return codegen::jit_module_builder::current_builder()->ir_builder().CreateUDiv(lhs_.eval(), rhs_.eval());
    case arithmetic_operation_type::smod:
        return codegen::jit_module_builder::current_builder()->ir_builder().CreateSRem(lhs_.eval(), rhs_.eval());
    case arithmetic_operation_type::umod:
        return codegen::jit_module_builder::current_builder()->ir_builder().CreateURem(lhs_.eval(), rhs_.eval());
    case arithmetic_operation_type::and_:
      return codegen::jit_module_builder::current_builder()->ir_builder().CreateAnd(lhs_.eval(), rhs_.eval());
    case arithmetic_operation_type::or_:
      return codegen::jit_module_builder::current_builder()->ir_builder().CreateOr(lhs_.eval(), rhs_.eval());
    case arithmetic_operation_type::xor_:
      return codegen::jit_module_builder::current_builder()->ir_builder().CreateXor(lhs_.eval(), rhs_.eval());
    }
  } else if (lhs_.isFloatType()) {
    switch (op_) {
    case arithmetic_operation_type::add:
      return codegen::jit_module_builder::current_builder()->ir_builder().CreateFAdd(lhs_.eval(), rhs_.eval());
    case arithmetic_operation_type::sub:
      return codegen::jit_module_builder::current_builder()->ir_builder().CreateFSub(lhs_.eval(), rhs_.eval());
    case arithmetic_operation_type::mul:
      return codegen::jit_module_builder::current_builder()->ir_builder().CreateFMul(lhs_.eval(), rhs_.eval());
    // TODO: floating point specific div/mod
    case arithmetic_operation_type::sdiv:
      return codegen::jit_module_builder::current_builder()->ir_builder().CreateFDiv(lhs_.eval(), rhs_.eval());
    case arithmetic_operation_type::smod:
      return codegen::jit_module_builder::current_builder()->ir_builder().CreateFRem(lhs_.eval(), rhs_.eval());
    case arithmetic_operation_type::udiv: [[fallthrough]];
    case arithmetic_operation_type::umod: [[fallthrough]];
    case arithmetic_operation_type::and_: [[fallthrough]];
    case arithmetic_operation_type::or_: [[fallthrough]];
    case arithmetic_operation_type::xor_: abort();
    }
  } else {
    llvm_unreachable("unimplemented");
  }
}

std::ostream& operator<<(std::ostream& os, arithmetic_operations const& ao) {
  auto symbol = [ao] {
    switch (ao.op_) {
    case arithmetic_operation_type::add: return "+";
    case arithmetic_operation_type::sub: return "-";
    case arithmetic_operation_type::mul: return "*";
    case arithmetic_operation_type::sdiv: return "s/";
    case arithmetic_operation_type::udiv: return "s/";
    case arithmetic_operation_type::smod: return "u%";
    case arithmetic_operation_type::umod: return "u%";
    case arithmetic_operation_type::and_: return "&";
    case arithmetic_operation_type::or_: return "|";
    case arithmetic_operation_type::xor_: return "^";
    }
  }();
  return os << '(' << ao.lhs_ << ' ' << symbol << ' ' << ao.rhs_ << ')';
}

} // namespace codegen::detail
