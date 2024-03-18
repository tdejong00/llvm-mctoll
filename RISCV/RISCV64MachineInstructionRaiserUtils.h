//===-- RISCV64MachineInstructionRaiserUtils.h ------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains the declaration of multiple utility functions regarding
// raising of machine instructions for use by llvm-mctoll.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_TOOLS_LLVM_MCTOLL_RISCV_RISCV64MACHINEINSTRUCTIONRAISERUTILS_H
#define LLVM_TOOLS_LLVM_MCTOLL_RISCV_RISCV64MACHINEINSTRUCTIONRAISERUTILS_H

#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/IR/Type.h"

namespace llvm {
namespace mctoll {
namespace riscv_utils {

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

/// Removes the prolog instructions from the basic block.
void removeProlog(MachineBasicBlock *MBB);

/// Removes the epilog instructions from the basic block.
void removeEpilog(MachineBasicBlock *MBB);

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

} // end namespace riscv_utils
} // end namespace mctoll
} // end namespace llvm

#endif // LLVM_TOOLS_LLVM_MCTOLL_RISCV_RISCV64MACHINEINSTRUCTIONRAISERUTILS_H