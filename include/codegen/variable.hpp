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

// We have another flavor of wrappers inside codegen::jit namespace. 
// IT does not take template arguments because we need to:
// 1. run it at runtime 
// 2. do codegen incrementally
// 3. need to store intemediary results.
struct type_reverse_lookup {
  static std::string name(llvm::Type *type) {
    if (type->isVoidTy()) {
      return "void";
    } else if (type->isIntegerTy(1)) {
      return "bool";
    } else if (type->isIntegerTy()) {
      int32_t bitwidth = type->getIntegerBitWidth();
      return fmt::format("s{}", bitwidth);
    }else if (type->isFloatTy()) {
      return "f32";
    } else {
      llvm_unreachable("unimplemented");
    }
  }

  static llvm::DIType* dbg(llvm::Type* type)  {
    std::string ty_name = type_reverse_lookup::name(type);
    if (type->isVoidTy()) {
      return nullptr;      
    } else if (type->isIntegerTy(1)) {
      // bool type
      return codegen::jit_module_builder::current_builder()->debug_builder().createBasicType(ty_name, 8,
                                                                                         llvm::dwarf::DW_ATE_boolean);
    } else if (type->isIntegerTy()) {
      assert(!type->isIntegerTy(1));
      // TODO: implement unsigned
      return codegen::jit_module_builder::current_builder()->debug_builder().createBasicType(
        ty_name, type->getIntegerBitWidth(), llvm::dwarf::DW_ATE_signed);

    } else if (type->isFloatTy()) {
      return codegen::jit_module_builder::current_builder()->debug_builder().createBasicType(ty_name, 32,
                                                                                         llvm::dwarf::DW_ATE_float);
    } else {
      llvm_unreachable("unimplemented");
    }
  }
};

class variable {
  llvm::Instruction* variable_;
  std::string name_;

  detail::variable_type type_;
  uint64_t bitwidth_;

  explicit variable(std::string const& n, detail::variable_type type, size_t bitwidth = 0) : name_(n) {
    auto& mb = *jit_module_builder::current_builder();
    auto& context = mb.context();

    llvm::Type * llvm_type = nullptr;
    switch (type) {
      using detail::variable_type;
      case variable_type::BoolTy:
        llvm_type = llvm::IntegerType::get(context, 1);
        break;
      case variable_type::IntTy:
        assert(bitwidth != 0 && "integer type cannot have zero bitwidth");
        llvm_type = llvm::IntegerType::get(context, bitwidth);
        break;
      case variable_type::FloatTy:
        llvm_type = llvm::Type::getFloatTy(context);
        break;
      default:
        llvm_unreachable("unimplemented");
    }
    assert(llvm_type);

    auto alloca_builder =
        llvm::IRBuilder<>(&mb.current_function()->getEntryBlock(), mb.current_function()->getEntryBlock().begin());
    variable_ = alloca_builder.CreateAlloca(llvm_type, nullptr, name_);

    auto line_no = mb.source_code_.add_line(fmt::format("{} {};", type_reverse_lookup::name(llvm_type), name_));
    auto& debug_builder = mb.debug_builder();
    auto dbg_variable = debug_builder.createAutoVariable(
        mb.source_code_.debug_scope(), name_, mb.source_code_.debug_file(), line_no, type_reverse_lookup::dbg(llvm_type));
    debug_builder.insertDeclare(variable_, dbg_variable, debug_builder.createExpression(),
                                mb.get_debug_location(line_no), mb.ir_builder().GetInsertBlock());
  }

public:
  template<int size>
  static variable variable_integer(std::string const &n) {
    return variable(n, variable_type::IntTy, size);
  }

  static variable variable_bool(std::string const& n) {
    return variable_integer<1>(n);
  }

  static variable variable_i32(std::string const &n) {
    return variable_integer<32>(n);
  }

  static variable variable_float(std::string const &n) {
    return variable(n, variable_type::FloatTy, sizeof(float));
  }

  // TODO: array, struct, double, string, byte types.
}; // class variable

} // namespace codegen
