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
#include "utils.hpp"

namespace codegen {

template<typename TrueBlockLambda, typename FalseBlockLambda>
inline void if_(value&& cnd, TrueBlockLambda&& tb, FalseBlockLambda&& fb) {
  assert(cnd.isBoolType());
  auto& mb = *jit_module_builder::current_builder();

  auto line_no = mb.source_code_.add_line(fmt::format("if ({}) {{", cnd));
  mb.ir_builder().SetCurrentDebugLocation(mb.get_debug_location(line_no));

  auto true_block = llvm::BasicBlock::Create(mb.context(), "true_block", mb.current_function());
  auto false_block = llvm::BasicBlock::Create(mb.context(), "false_block");
  auto merge_block = llvm::BasicBlock::Create(mb.context(), "merge_block");

  mb.ir_builder().CreateCondBr(cnd.eval(), true_block, false_block);

  mb.ir_builder().SetInsertPoint(true_block);
  mb.source_code_.enter_scope();

  assert(!mb.exited_block_);
  tb();
  mb.source_code_.leave_scope();

  line_no = mb.source_code_.add_line("} else {");

  if (!mb.exited_block_) {
    mb.ir_builder().SetCurrentDebugLocation(mb.get_debug_location(line_no));
    mb.ir_builder().CreateBr(merge_block);
  }
  mb.exited_block_ = false;

  mb.current_function()->getBasicBlockList().push_back(false_block);
  mb.ir_builder().SetInsertPoint(false_block);

  mb.source_code_.enter_scope();
  fb();
  mb.source_code_.leave_scope();

  line_no = mb.source_code_.add_line("}");

  if (!mb.exited_block_) {
    mb.ir_builder().SetCurrentDebugLocation(mb.get_debug_location(line_no));
    mb.ir_builder().CreateBr(merge_block);
  }
  mb.exited_block_ = false;

  mb.current_function()->getBasicBlockList().push_back(merge_block);
  mb.ir_builder().SetInsertPoint(merge_block);
}

template<typename TrueBlockLambda>
inline void if_(value&& cnd, TrueBlockLambda&& tb) {
  assert(cnd.isBoolType());
  auto& mb = *jit_module_builder::current_builder();

  auto line_no = mb.source_code_.add_line(fmt::format("if ({}) {{", cnd));
  mb.ir_builder().SetCurrentDebugLocation(mb.get_debug_location(line_no));

  auto true_block = llvm::BasicBlock::Create(mb.context(), "true_block", mb.current_function());
  auto merge_block = llvm::BasicBlock::Create(mb.context(), "merge_block");

  mb.ir_builder().CreateCondBr(cnd.eval(), true_block, merge_block);

  mb.ir_builder().SetInsertPoint(true_block);

  mb.source_code_.enter_scope();
  assert(!mb.exited_block_);
  tb();
  mb.source_code_.leave_scope();

  line_no = mb.source_code_.add_line("}");

  if (!mb.exited_block_) {
    mb.ir_builder().SetCurrentDebugLocation(mb.get_debug_location(line_no));
    mb.ir_builder().CreateBr(merge_block);
  }
  mb.exited_block_ = false;

  mb.current_function()->getBasicBlockList().push_back(merge_block);
  mb.ir_builder().SetInsertPoint(merge_block);
}

value call(function_ref const& fn, llvm::ArrayRef<value> args);

void void_call(function_ref const& fn, llvm::ArrayRef<value> args);


auto load(value ptr);

void store(value v, value ptr);

template<typename CondLambda, typename BodyLambda>
inline void while_(CondLambda cnd_fn, BodyLambda && bdy) {
  auto& mb = *jit_module_builder::current_builder();

  //assert(cnd_fn.isBoolType());

  auto line_no = mb.source_code_.current_line() + 1;
  mb.ir_builder().SetCurrentDebugLocation(mb.get_debug_location(line_no));

  auto while_continue = llvm::BasicBlock::Create(mb.context(), "while_continue", mb.current_function());
  auto while_iteration = llvm::BasicBlock::Create(mb.context(), "while_iteration");
  auto while_break = llvm::BasicBlock::Create(mb.context(), "while_break");

  auto parent_loop = std::exchange(mb.current_loop_, jit_module_builder::loop{while_continue, while_break});

  mb.ir_builder().CreateBr(while_continue);
  mb.ir_builder().SetInsertPoint(while_continue);

  auto cond = cnd_fn();
  mb.ir_builder().CreateCondBr(cond, while_iteration, while_break);
  mb.source_code_.add_line(fmt::format("while ({}) {{", cond));

  mb.source_code_.enter_scope();

  mb.current_function()->getBasicBlockList().push_back(while_iteration);
  mb.ir_builder().SetInsertPoint(while_iteration);

  assert(!mb.exited_block_);
  bdy();

  mb.source_code_.leave_scope();

  line_no = mb.source_code_.add_line("}");

  if (!mb.exited_block_) {
    mb.ir_builder().SetCurrentDebugLocation(mb.get_debug_location(line_no));
    mb.ir_builder().CreateBr(while_continue);
  }
  mb.exited_block_ = false;

  mb.current_function()->getBasicBlockList().push_back(while_break);
  mb.ir_builder().SetInsertPoint(while_break);

  mb.current_loop_ = parent_loop;
}

void break_();

inline void continue_();

inline value true_() {
  return constant(true);
}

inline value false_() {
  return constant(false);
}

void return_();

void return_(value v);

} // namespace codegen
