//===-- RISCV64MachineInstructionUtils.h ------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains the declaration of multiple utility functions regarding
// machine instructions and machine basic blocks for use by llvm-mctoll.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_TOOLS_LLVM_MCTOLL_RISCV_RISCV64_RISCV64MACHINEINSTRUCTIONUTILS_H
#define LLVM_TOOLS_LLVM_MCTOLL_RISCV_RISCV64_RISCV64MACHINEINSTRUCTIONUTILS_H

#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/IR/Type.h"

namespace llvm {
namespace mctoll {
namespace RISCV64MachineInstructionUtils {

/// Gets the default integer type.
IntegerType *getDefaultIntType(MachineFunction &MF);
/// Gets the default pointer type.
PointerType *getDefaultPtrType(MachineFunction &MF);

/// Determines whether the machine instruction is a part of the prolog.
bool isPrologInstruction(const MachineInstr &MI);
/// Determines whether the machine instruction is a part of the epilog.
bool isEpilogInstruction(const MachineInstr &MI);

/// Returns the iterator of the first instruction after the prolog.
MachineBasicBlock::const_instr_iterator
skipProlog(const MachineBasicBlock &MBB);

/// Finds the instruction in the basic block which has the given opcode.
/// Only search up until the given end iterator.
MachineBasicBlock::const_reverse_instr_iterator
findInstructionByOpcode(const MachineBasicBlock &MBB, unsigned Op,
                        MachineBasicBlock::const_reverse_instr_iterator EndIt);

/// Finds the instruction in the basic block which defines the given register
/// number. Only search up until the given end iterator.
MachineBasicBlock::const_reverse_instr_iterator
findInstructionByRegNo(const MachineBasicBlock &MBB, unsigned RegNO,
                       MachineBasicBlock::const_reverse_instr_iterator EndIt);

} // namespace RISCV64MachineInstructionUtils
} // namespace mctoll
} // namespace llvm

#endif // LLVM_TOOLS_LLVM_MCTOLL_RISCV_RISCV64_RISCV64MACHINEINSTRUCTIONUTILS_H
