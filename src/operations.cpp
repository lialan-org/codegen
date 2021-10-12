
#include "codegen/arithmetic_ops.hpp"
#include "codegen/relational_ops.hpp"

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
  }
  abort();
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
    abort();
  }();
  return os << '(' << ao.lhs_ << ' ' << symbol << ' ' << ao.rhs_ << ')';
}

std::ostream& operator<<(std::ostream& os, pointer_arithmetic_operations const& ao) {
  auto symbol = [ao] {
    switch (ao.op_) {
    case pointer_arithmetic_operation_type::add: return '+';
    case pointer_arithmetic_operation_type::sub: return '-';
    default: assert(false); break;
    }
  }();
  return os << '(' << ao.lhs_ << ' ' << symbol << ' ' << ao.rhs_ << ')';
}

llvm::Value* relational_operations::eval() const {
  if (lhs_.isIntegerType()) {
    switch (op_) {
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
    switch (op_) {
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
  }
  abort();
}

std::ostream& operator<<(std::ostream& os, relational_operations const& ro) {
  auto symbol = [ro] {
    switch (ro.op_) {
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
    abort();
  }();
  return os << '(' << ro.lhs_ << ' ' << symbol << ' ' << ro.rhs_ << ')';
}

} // namespace codegen::detail
