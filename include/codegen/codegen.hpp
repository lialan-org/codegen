#pragma once

#include "module_builder.hpp"
#include "module.hpp"
#include "compiler.hpp"
#include "types.hpp"
#include "variable.hpp"
#include "literals.hpp"
#include "relational_ops.hpp"
#include "statements.hpp"
#include "arithmetic_ops.hpp"
#include "builtin.hpp"
#include "utils.hpp"

#include <llvm/Support/InitLLVM.h>
#include <llvm/Support/TargetSelect.h>

#define INIT_LLVM_ENV(argc, argv)           \
  llvm::InitLLVM x(argc, argv);             \
  llvm::InitializeNativeTarget();           \
  llvm::InitializeNativeTargetAsmPrinter()
