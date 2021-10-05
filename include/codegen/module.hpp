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

namespace codegen {

class module {
  std::unique_ptr<llvm::orc::LLJIT> lljit_;
  llvm::orc::MangleAndInterner mangle_;

  std::unordered_map<std::string, std::function<void*>> function_entries_;

private:
  module(std::unique_ptr<llvm::orc::LLJIT> lljit, llvm::orc::MangleAndInterner const& mangle)
      : lljit_(std::move(lljit)), mangle_(std::move(mangle)) {}

  friend class module_builder;

public:
  module(module const&) = delete;
  module(module&&) = delete;

  template<typename ReturnType, typename... Arguments>
  auto get_address(std::string const &name) {
    auto entry_pointer = cantFail(lljit_->lookup(fn.name()));
    return llvm::jitTargetAddressToFunction<ReturnType (*)(Arguments...)>(entry_pointer.getAddress());
  }
};

} // namespace codegen
