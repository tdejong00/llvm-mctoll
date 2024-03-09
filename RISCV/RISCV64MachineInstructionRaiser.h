//===-- RISCV64MachineInstructionRaiser.h -----------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains the declaration of RISCV64MachineInstructionRaiser
// class for use by llvm-mctoll.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_TOOLS_LLVM_MCTOLL_RISCV_RISCV64MACHINEINSTRUCTIONRAISER_H
#define LLVM_TOOLS_LLVM_MCTOLL_RISCV_RISCV64MACHINEINSTRUCTIONRAISER_H

#include "Raiser/MachineInstructionRaiser.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFunction.h"

namespace llvm {
namespace mctoll {

class RISCV64MachineInstructionRaiser : public MachineInstructionRaiser {
public:
  RISCV64MachineInstructionRaiser() = delete;
  RISCV64MachineInstructionRaiser(MachineFunction &MF, const ModuleRaiser *MR, MCInstRaiser *MCIR);

  bool raise() override;
  FunctionType *getRaisedFunctionPrototype() override;
  int getArgumentNumber(unsigned PReg) override;
  Value *getRegOrArgValue(unsigned PReg, int MBBNo) override;
  bool buildFuncArgTypeVector(const std::set<MCPhysReg> &, std::vector<Type *> &) override;

private:
  /// Finds the instruction in the specified basic block which has the specified opcode.
  MachineBasicBlock::const_reverse_iterator findInstruction(unsigned Op, const MachineBasicBlock &MBB);

  /// Finds the instruction in the specified basic block which defines the specified register.
  MachineBasicBlock::const_reverse_iterator findInstruction(unsigned Reg, const MachineBasicBlock &MBB, MachineBasicBlock::const_reverse_iterator EndIt);
  
  /// Determines the parameter types of the current MachineFunction of this raiser.
  std::vector<Type *> determineArgumentTypes();
  
  /// Determines the return type of the current MachineFunction of this raiser.
  Type *determineReturnType();
};

} // end namespace mctoll
} // end namespace llvm

#endif // LLVM_TOOLS_LLVM_MCTOLL_RISCV64_RISCV64MACHINEINSTRUCTIONRAISER_H
