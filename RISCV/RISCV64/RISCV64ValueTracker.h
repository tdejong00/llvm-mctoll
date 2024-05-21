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

#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Value.h"
#include <unordered_map>

namespace llvm {
namespace mctoll {

/// Represents a register definition containing the machine instruction
/// which defines the register and the accompanying value of the definition.
struct RegisterDefinition {
  signed MBBNo;
  const MachineInstr &MI;
  Value *Val;
};

// Forward declaration of RISCV64MachineInstructionRaiser
class RISCV64MachineInstructionRaiser;

/// Keeps track of SSA values currently asigned to registers
/// and stack slots during the raising of a machine function.
class RISCV64ValueTracker {
public:
  RISCV64ValueTracker() = delete;
  RISCV64ValueTracker(RISCV64MachineInstructionRaiser *MIR);

  /// Gets the SSA value currently assigned to the specified register, by first
  /// looking for a local definition. If the register is not defined in the
  /// specified basic block, looks at the predecessors for a definition.
  /// When all predecessors define the specified register, the register
  /// is promoted to a stack slot, all predecessors will store to that stack
  /// slot, and load instruction from that stack slot will be returned.
  Value *getRegValue(int MBBNo, unsigned int RegNo);

  /// Sets the SSA value currently assigned to the specified register.
  void setRegValue(int MBBNo, unsigned int RegNo, Value *Val);

  /// Gets the AllocaInst currently functioning as the specified stack slot.
  /// When the stack slot does not yet exist, it is created using the specified
  /// type. If the type is not specified, i64 is used by default.
  Value *getStackSlot(int StackOffset, Type *AllocaTy = nullptr);

private:
  /// Returns the last instruction which defines the register,
  /// or instr_rend if it does not define the register at all.
  MachineBasicBlock::const_reverse_instr_iterator
  getFinalDefinition(unsigned int RegNo, const MachineBasicBlock *MBB);

  /// Returns the register definitions for the specified register
  /// made by the predecessors of the specified basic block.
  std::vector<RegisterDefinition> getDefinitions(unsigned int RegNo,
                                                 const MachineBasicBlock *MBB);

  /// Determines the type for the stack slot for the promoted register
  /// based on the different definitions of the branches.
  Type *getStackSlotType(const std::vector<RegisterDefinition> &Defs);

  RISCV64MachineInstructionRaiser *MIR;
  MachineFunction &MF;
  LLVMContext &C;

  using RegisterValueMap = std::unordered_map<unsigned int, Value *>;
  using MBBRegisterValuesMap = std::unordered_map<int, RegisterValueMap>;

  /// Mapping from MBB number to a mapping from register number
  /// to its current value. Each MBB has its own register-value
  /// map to facilitate different values for branches.
  MBBRegisterValuesMap MBBRegValues;

  /// Mapping from stack offset to the current
  /// AllocaInst representing the stack slot.
  std::unordered_map<int, Value *> StackValues;
};

} // namespace mctoll
} // namespace llvm

#endif // LLVM_TOOLS_LLVM_MCTOLL_RISCV_RISCV64_RISCV64VALUETRACKER_H