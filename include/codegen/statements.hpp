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

using BlockLambda = std::function<void()>;

inline void if_(value&& cnd, BlockLambda tb, BlockLambda fb);

inline void if_(value&& cnd, BlockLambda&& tb);

value call(function_ref const& fn, llvm::ArrayRef<value> args);

void void_call(function_ref const& fn, llvm::ArrayRef<value> args);


auto load(value ptr);

void store(value v, value ptr);

void while_(value&& cnd, BlockLambda&& body);

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
