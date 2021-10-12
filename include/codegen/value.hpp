#pragma once

#include "llvm/IR/Value.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/DerivedTypes.h"

#include <iostream>

namespace codegen {

class value {
protected:
  llvm::Value* value_;
  std::string name_;

public:
  explicit value(llvm::Value* v, std::string const& n)
    : value_(v), name_(n) {}

  value(value const&) = default;
  value(value&&) = default;
  void operator=(value const&) = delete;
  void operator=(value&&) = delete;

  virtual operator llvm::Value *() const noexcept { return value_; }

  virtual llvm::Type* get_type() const { return value_->getType(); }

  virtual llvm::Value* eval() const { return value_; }
  virtual std::string get_name() const { return name_; }

  friend std::ostream& operator<<(std::ostream& os, value v) { return os << v.get_name(); }

  bool isPointerType() const {
    return get_type()->isPointerTy();
  }
  bool isIntegerType() const {
    return get_type()->isIntegerTy();
  }
  bool isBoolType() const {
    return get_type()->isIntegerTy(1);
  }
  bool isFloatType() const {
    return get_type()->isFloatTy();
  }

  // TODO: LLVM IR does not distinguish sign or unsigned values. so we need to carry
  // extra info.
  /*
  bool isSignedIntegerType() const { }
  */

  bool getPointerElementType() const {
    assert(isPointerType());
    llvm::PointerType * ptr_type = llvm::dyn_cast<llvm::PointerType>(get_type());
    return ptr_type->getElementType();
  }
};

} // namespace codegen
