//===-- RISCV64ValueTracker.h -----------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains the declaration of RISCV64ValueTracker class for use by
// llvm-mctoll.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_TOOLS_LLVM_MCTOLL_RISCV_RISCV64_RISCV64VALUETRACKER_H
#define LLVM_TOOLS_LLVM_MCTOLL_RISCV_RISCV64_RISCV64VALUETRACKER_H

#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Value.h"
#include <unordered_map>

namespace llvm {
namespace mctoll {

/// Keeps track of SSA values currently asigned to registers
/// and stack slots during the raising of a machine function.
class RISCV64ValueTracker {
public:
  RISCV64ValueTracker() = delete;
  RISCV64ValueTracker(MachineFunction &MF)
      : MF(MF), C(MF.getFunction().getContext()) {}

  /// Gets the SSA value currently assigned to the specified register.
  Value *getRegValue(signed MBBNo, unsigned RegNo);

  /// Sets the SSA valu currently assigned to the specified register.
  void setRegValue(signed MBBNo, unsigned RegNo, Value *Val);

  /// Gets the SSA value currently assigned to the specified stack slot.
  Value *getStackValue(signed StackOffset);

  /// Sets the SSA value currently assigned to the specified stack slot.
  void setStackValue(signed StackOffset, Value *Val);

private:
  MachineFunction &MF;
  LLVMContext &C;

  using RegisterValueMap = std::unordered_map<unsigned, Value *>;
  using MBBRegisterValuesMap = std::unordered_map<signed, RegisterValueMap>;

  /// Mapping from MBB number to a mapping from register number
  /// to its current value. Each MBB has its own register-value
  /// map to facilitate different values for branches.
  MBBRegisterValuesMap MBBRegValues;

  /// Mapping from stack offset to current value assigned to stack slot.
  std::unordered_map<signed, Value *> StackValues;
};

} // namespace mctoll
} // namespace llvm

#endif // LLVM_TOOLS_LLVM_MCTOLL_RISCV_RISCV64_RISCV64VALUETRACKER_H