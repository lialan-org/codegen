#include "codegen/value.hpp"
#include "codegen/variable.hpp"

namespace codegen {

variable::variable(std::string const& n, llvm::Type *llvm_type) : value(nullptr, n) {
  auto& mb = *jit_module_builder::current_builder();

  auto alloca_builder =
      llvm::IRBuilder<>(&mb.current_function()->getEntryBlock(), mb.current_function()->getEntryBlock().begin());
  value_ = alloca_builder.CreateAlloca(llvm_type, nullptr, name_);

  auto line_no = mb.source_code_.add_line(fmt::format("{} {};", detail::type_reverse_lookup::name(llvm_type), name_));
  auto& debug_builder = mb.debug_builder();
  auto dbg_variable = debug_builder.createAutoVariable(
      mb.source_code_.debug_scope(), name_, mb.source_code_.debug_file(), line_no, detail::type_reverse_lookup::dbg(llvm_type));
  debug_builder.insertDeclare(value_, dbg_variable, debug_builder.createExpression(),
                              mb.get_debug_location(line_no), mb.ir_builder().GetInsertBlock());
}

void variable::set(value const& v) {
  //assert(v.get_type() == this->get_type());
  auto& mb = *jit_module_builder::current_builder();
  auto line_no = mb.source_code_.add_line(fmt::format("{} = {};", name_, v));
  mb.ir_builder().SetCurrentDebugLocation(mb.get_debug_location(line_no));
  mb.ir_builder().CreateAlignedStore(v.eval(), value_, llvm::MaybeAlign());
}

value variable::get() const {
  auto v = jit_module_builder::current_builder()->ir_builder().CreateAlignedLoad(
      value_, llvm::MaybeAlign());
  return value{v, name_};
}

} // namespace codegen
