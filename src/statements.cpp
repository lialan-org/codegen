#include "codegen/statements.hpp"

namespace codegen {

value call(function_ref const& fn, llvm::ArrayRef<value> args) {
  auto& mb = *jit_module_builder::current_builder();

  llvm::FunctionType* func_type = fn.get_function_type();
  auto *return_type = func_type->getReturnType();
  assert(!return_type->isVoidTy());

  {
    auto str = std::stringstream{};
    str << fn.name() << "_ret = " << fn.name() << "(";

    for (auto &v : args) {
      str << fmt::format("{}, ", v);
    }

    str << ");";
    auto line_no = mb.source_code_.add_line(str.str());
    mb.ir_builder().SetCurrentDebugLocation(mb.get_debug_location(line_no));
  }

  auto values = std::vector<llvm::Value*>{};
  for (auto &v : args) {
    values.emplace_back(v.eval());
  }

  auto ret = mb.ir_builder().CreateCall(fn, values);
  return value{ret, fmt::format("{}_ret", fn.name())};
}

void void_call(function_ref const& fn, llvm::ArrayRef<value> args) {
  auto& mb = *jit_module_builder::current_builder();

  llvm::FunctionType* func_type = fn.get_function_type();
  auto *return_type = func_type->getReturnType();
  assert(return_type->isVoidTy());

  {
    auto str = std::stringstream{};
    str << fn.name() << "(";

    for (auto &v : args) {
      str << fmt::format("{}, ", v);
    }

    str << ");";
    auto line_no = mb.source_code_.add_line(str.str());
    mb.ir_builder().SetCurrentDebugLocation(mb.get_debug_location(line_no));
  }

  auto values = std::vector<llvm::Value*>{};
  for (auto &v : args) {
    values.emplace_back(v.eval());
  }

  mb.ir_builder().CreateCall(fn, values);
}

auto load(value ptr) {
  auto *elem_type = ptr.get_type()->getPointerElementType();

  auto& mb = *jit_module_builder::current_builder();

  auto id = fmt::format("val{}", ::codegen::detail::id_counter++);

  auto line_no = mb.source_code_.add_line(fmt::format("{} = *{}", id, ptr));
  mb.ir_builder().SetCurrentDebugLocation(mb.get_debug_location(line_no));
  auto v = mb.ir_builder().CreateAlignedLoad(ptr.eval(), llvm::MaybeAlign());

  auto dbg_value = mb.debug_builder().createAutoVariable(
      mb.source_code_.debug_scope(), id, mb.source_code_.debug_file(),
      line_no, ::codegen::detail::type_reverse_lookup::dbg(elem_type));
  mb.debug_builder().insertDbgValueIntrinsic(v, dbg_value, mb.debug_builder().createExpression(),
                                             mb.get_debug_location(line_no), mb.ir_builder().GetInsertBlock());
  return value{v, id};
}

void store(value v, value ptr) {
  assert(ptr.isPointerType());
  // TODO: check element type the same
  auto& mb = *jit_module_builder::current_builder();

  auto line_no = mb.source_code_.add_line(fmt::format("*{} = {}", ptr, v));
  mb.ir_builder().SetCurrentDebugLocation(mb.get_debug_location(line_no));
  mb.ir_builder().CreateAlignedStore(v.eval(), ptr.eval(), llvm::MaybeAlign());
}

void break_() {
  auto& mb = *jit_module_builder::current_builder();
  assert(mb.current_loop_.break_block_);

  mb.exited_block_ = true;

  auto line_no = mb.source_code_.add_line("break;");
  mb.ir_builder().SetCurrentDebugLocation(mb.get_debug_location(line_no));

  mb.ir_builder().CreateBr(mb.current_loop_.break_block_);
}

void continue_() {
  auto& mb = *jit_module_builder::current_builder();
  assert(mb.current_loop_.continue_block_);

  mb.exited_block_ = true;

  auto line_no = mb.source_code_.add_line("continue;");
  mb.ir_builder().SetCurrentDebugLocation(mb.get_debug_location(line_no));

  mb.ir_builder().CreateBr(mb.current_loop_.continue_block_);
}

void return_() {
  auto& mb = *jit_module_builder::current_builder();
  auto line_no = mb.source_code_.add_line("return;");
  mb.exited_block_ = true;
  mb.ir_builder().SetCurrentDebugLocation(mb.get_debug_location(line_no));
  mb.ir_builder().CreateRetVoid();
}

void return_(value v) {
  auto& mb = *jit_module_builder::current_builder();
  mb.exited_block_ = true;
  auto line_no = mb.source_code_.add_line(fmt::format("return {};", v));
  mb.ir_builder().SetCurrentDebugLocation(mb.get_debug_location(line_no));
  mb.ir_builder().CreateRet(v.eval());
}

} // namespace codegen
