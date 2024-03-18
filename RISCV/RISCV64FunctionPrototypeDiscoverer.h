//===-- RISCV64FunctionPrototypeDiscoverer.h --------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains the declaration of the RISCV64FunctionPrototypeDiscoverer
// class for use by llvm-mctoll.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_TOOLS_LLVM_MCTOLL_RISCV_RISCV64FUNCTIONPROTOTYPEDISCOVERER_H
#define LLVM_TOOLS_LLVM_MCTOLL_RISCV_RISCV64FUNCTIONPROTOTYPEDISCOVERER_H

#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/LLVMContext.h"
#include <vector>

namespace llvm {
namespace mctoll {

class RISCV64FunctionPrototypeDiscoverer {
public:
  RISCV64FunctionPrototypeDiscoverer(MachineFunction &MF)
      : MF(MF), C(MF.getFunction().getContext()) {}

  /// Discovers the function prototype of the machine function.
  Function *discoverFunctionPrototype() const;

  /// Discovers the return type of the machine function.
  Type *discoverReturnType() const;

  /// Discovers the parameter types of the machine function.
  std::vector<Type *> discoverArgumentTypes() const;

private:
  MachineFunction &MF;
  LLVMContext &C;
};

} // end namespace mctoll
} // end namespace llvm

#endif // LLVM_TOOLS_LLVM_MCTOLL_RISCV_RISCV64FUNCTIONPROTOTYPEDISCOVERER_H