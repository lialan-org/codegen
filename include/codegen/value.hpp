#pragma once

#include "module_builder.hpp"

namespace codegen {

template<typename Type> class value {
  llvm::Value* value_;
  std::string name_;

public:
  static_assert(!std::is_const_v<Type>);
  static_assert(!std::is_volatile_v<Type>);

  explicit value(llvm::Value* v, std::string const& n) : value_(v), name_(n) {}

  value(value const&) = default;
  value(value&&) = default;
  void operator=(value const&) = delete;
  void operator=(value&&) = delete;

  using value_type = Type;

  operator llvm::Value*() const noexcept { return value_; }

  llvm::Value* eval() const { return value_; }

  friend std::ostream& operator<<(std::ostream& os, value v) { return os << v.name_; }
};

} // namespace codegen
