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
#include "codegen/types.hpp"
#include "codegen/value.hpp"

namespace codegen::builtin {

using namespace codegen;

void register_printf(llvm::LLVMContext &context, llvm::Module &module);

template<typename T>
inline value bitcast(value src) {
  llvm::Type* target_type = detail::type_reverse_lookup::type<T>();
  llvm::Type* original_type = src.get_type();

  if (target_type == original_type) {
    return src;
  }

  auto& mb = *jit_module_builder::current_builder();
  auto casted_value = mb.ir_builder().CreateBitCast(src.eval(), target_type);
  return value{casted_value, src.get_name() + "_casted"}; 
}


value gep(value src, size_t index);
void memcpy(value dst, value src, value n);
value memcmp(value src1, value src2, value n);


} // namespace codegen::builtin
