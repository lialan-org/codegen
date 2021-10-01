#pragma once

#if defined (__GNUC__)
#pragma GCC diagnostic ignored "-Wredundant-move"
#elif defined (__clang__)
#pragma GCC diagnostic ignored "-Wredundant-move"
#endif

#include "arithmetic_ops.hpp"
#include "builtin.hpp"
#include "compiler_context.hpp"
#include "literals.hpp"
#include "module.hpp"
#include "module_builder.hpp"
#include "relational_ops.hpp"
#include "statements.hpp"
#include "types.hpp"
#include "utils.hpp"
#include "variable.hpp"

#include <llvm/Support/InitLLVM.h>
#include <llvm/Support/TargetSelect.h>

#if defined (__GNUC__)
#pragma GCC diagnostic pop
#elif defined (__clang__)
#pragma clang diagnostic pop
#endif

#define INIT_LLVM_TARGET()                                                                                             \
  llvm::InitializeNativeTarget();                                                                                      \
  llvm::InitializeNativeTargetAsmPrinter()

#define INIT_LLVM_ENV(argc, argv)                                                                                      \
  llvm::InitLLVM x(argc, argv);                                                                                        \
  INIT_LLVM_TARGET()

