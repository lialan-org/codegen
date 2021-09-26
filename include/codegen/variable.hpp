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

template<typename T>
concept Variable = !std::is_const_v<T> && !std::is_volatile_v<T>;

template<Variable Type>
class variable {
  llvm::Instruction* variable_;
  std::string name_;

public:
  explicit variable(std::string const& n) : name_(n) {
    auto& mb = *detail::current_builder;

    auto alloca_builder = llvm::IRBuilder<>(&mb.current_function()->getEntryBlock(), mb.current_function()->getEntryBlock().begin());
    variable_ = alloca_builder.CreateAlloca(detail::type<Type>::llvm(), nullptr, name_);

    auto line_no = mb.source_code_.add_line(fmt::format("{} {};", detail::type<Type>::name(), name_));
    auto &debug_builder = mb.debug_builder();
    auto dbg_variable =
        debug_builder.createAutoVariable(mb.source_code_.debug_scope(), name_, mb.source_code_.debug_file(), line_no, detail::type<Type>::dbg());
    debug_builder.insertDeclare(variable_, dbg_variable, debug_builder.createExpression(),
                                  mb.get_debug_location(line_no), mb.ir_builder().GetInsertBlock());
  }

  template<typename Value>
  explicit variable(std::string const& n, Value const& v) : variable(n) { set<Value>(v); }

  variable(variable const&) = delete;
  variable(variable&&) = delete;

  value<Type> get() const {
    auto v = detail::current_builder->ir_builder().CreateAlignedLoad(variable_, llvm::MaybeAlign(detail::type<Type>::alignment));
    return value<Type>{v, name_};
  }

  template<typename V>
  void set(V const& v) requires IsValue<V> && std::same_as<Type, typename V::value_type> {
    auto& mb = *detail::current_builder;
    auto line_no = mb.source_code_.add_line(fmt::format("{} = {};", name_, v));
    mb.ir_builder().SetCurrentDebugLocation(mb.get_debug_location(line_no));
    mb.ir_builder().CreateAlignedStore(v.eval(), variable_, llvm::MaybeAlign(detail::type<Type>::alignment));
  }

  template<typename T = Type, typename Value>
  typename std::enable_if_t<std::is_array_v<T>, void> set(Value const& v) {

  }

  // TODO
  // address-of operator gets you the pointer to the variable.
  //Type *operator&() { }

  template<typename T = Type, typename Value>
	value<std::remove_all_extents_t<T>>
  operator[](Value const& v) & requires IsArray<T> && LLVMIntegralType<typename Value::value_type> {
    using ElementType = typename std::remove_all_extents_t<T>;

    auto& mb = *detail::current_builder;

    auto idx = v.eval();

    if constexpr (sizeof(typename Value::value_type) < sizeof(uint64_t)) {
      if constexpr (std::is_unsigned_v<typename Value::value_type>) {
        idx = mb.ir_builder().CreateZExt(idx, detail::type<uint64_t>::llvm());
      } else {
        idx = mb.ir_builder().CreateSExt(idx, detail::type<int64_t>::llvm());
      }
    }

    auto elem_ptr = mb.ir_builder().CreateInBoundsGEP(variable_, idx);

    auto temp_storage = mb.ir_builder().CreateAlloca(detail::type<ElementType>::llvm(), nullptr, "temporary_storage");

    auto load = mb.ir_builder().CreateAlignedLoad(elem_ptr, llvm::MaybeAlign(detail::type<ElementType>::alignment));

    return value<std::remove_all_extents_t<T>>{load, "test_name"};
  }

  template<typename T = Type, typename IndexValue, typename Value>
  value<std::remove_all_extents_t<T>>
  setElem(IndexValue const& idx_v, Value const&& value_v)
      requires IsArray<T> &&
               LLVMIntegralType<typename IndexValue::value_type> && 
               std::same_as<std::remove_all_extents_t<T>, typename Value::value_type>  {
    auto& mb = *detail::current_builder;
    auto idx = idx_v.eval();

    if constexpr (sizeof(typename IndexValue::value_type) < sizeof(uint64_t)) {
      if constexpr (std::is_unsigned_v<typename IndexValue::value_type>) {
        idx = mb.ir_builder().CreateZExt(idx, detail::type<uint64_t>::llvm());
      } else {
        idx = mb.ir_builder().CreateSExt(idx, detail::type<int64_t>::llvm());
      }
    }

    auto elem = mb.ir_builder().CreateInBoundsGEP(variable_, idx);
    mb.ir_builder().CreateAlignedStore(value_v.eval(), idx, llvm::MaybeAlign(detail::type<typename Value::value_type>::alignment));
    return std::move(value_v);
  }

  // lvalue
  /*
  template<typename T = Type, typename Value>
  typename std::enable_if_t<std::is_array_v<T> &&
                            std::is_integral_v<typename Value::value_type>,
	    value<std::remove_all_extents_t<T>>>
  operator[](Value const& v) && {
    //static_assert(sizeof(Value::value_type) < sizeof(int64_t));
    auto& mb = *detail::current_builder;

    auto idx = v.eval();

    if constexpr (sizeof(typename Value::value_type) < sizeof(uint64_t)) {
      if constexpr (std::is_unsigned_v<Value::value_type>) {
        idx = mb.ir_builder().CreateZExt(idx, detail::type<uint64_t>::llvm());
      } else {
        idx = mb.ir_builder().CreateSExt(idx, detail::type<int64_t>::llvm());
      }
    }

    auto elem = mb.ir_builder().CreateInBoundsGEP(variable_, idx);
    std::string value_name = fmt::format("{}[{}]", name_, idx.name_);
    return value<std::remove_all_extents_t<T>>{elem, value_name};
  }
  */


};

} // namespace codegen
