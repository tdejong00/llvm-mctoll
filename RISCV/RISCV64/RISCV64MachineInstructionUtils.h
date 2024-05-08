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
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Type.h"
#include <algorithm>
#include <cstdint>

namespace llvm {
namespace mctoll {

using BinaryOps = llvm::BinaryOperator::BinaryOps;
using Predicate = llvm::CmpInst::Predicate;

/// Contains utility functions regarding machine instructions and finding
/// machine instructions within basic blocks.
namespace RISCV64MachineInstructionUtils {

/// Represents different instruction types.
enum class InstructionType {
  NOP,
  BINOP,
  MOVE,
  LOAD,
  STORE,
  GLOBAL,
  CALL,
  RETURN,
  UNCONDITIONAL_BRANCH,
  CONDITIONAL_BRANCH,
  UNKNOWN
};

/// Gets the default type for machine instruction using the given LLVM context,
/// based on if the given machine instruction loads or stores a pointer.
Type *getDefaultType(LLVMContext &C, const MachineInstr &MI);
/// Gets the default integer type using the given LLVM context.
IntegerType *getDefaultIntType(LLVMContext &C);
/// Gets the default pointer type using the given LLVM context.
PointerType *getDefaultPtrType(LLVMContext &C);

/// Creates a ConstantInt representing a GEP index, based on the given pointer
/// offset (number of bytes). The operands of the GEP instructions represent
/// indices and not number of bytes.
ConstantInt *toGEPIndex(LLVMContext &C, uint64_t Offset);

/// Determines the instruction type of the opcode.
InstructionType getInstructionType(unsigned Op);
/// Converts the opcode to a binary operation.
BinaryOps toBinaryOperation(unsigned Op);
/// Converts the opcode to a compare predicate.
Predicate toPredicate(unsigned Op);

/// Determines whether the opcode is an ADDI instruction.
bool isAddI(unsigned Op);

/// Determines whether the machine instruction is a part of the prolog.
bool isPrologInstruction(const MachineInstr &MI);
/// Determines whether the machine instruction is a part of the epilog.
bool isEpilogInstruction(const MachineInstr &MI);

/// Returns the iterator of the first instruction
/// after the prolog of the machine basic block.
MachineBasicBlock::const_instr_iterator
skipProlog(const MachineBasicBlock &MBB);

/// Determines if the register defined by the given machine instruction
/// is the final definition of that register within the machine basic block
/// which the machine instruction resides in.
bool isFinalDefinition(const MachineInstr &MI);

/// Finds the instruction in the machine basic block which has
/// the given opcode. Only searches up until the given end iterator.
MachineBasicBlock::const_reverse_instr_iterator
findInstructionByOpcode(const MachineBasicBlock &MBB, unsigned Op,
                        MachineBasicBlock::const_reverse_instr_iterator EndIt);

/// Finds the instruction in the machine basic block which defines the
/// given register number. Only searches up until the given end iterator.
MachineBasicBlock::const_reverse_instr_iterator
findInstructionByRegNo(const MachineBasicBlock &MBB, unsigned RegNo,
                       MachineBasicBlock::const_reverse_instr_iterator EndIt);

/// A struct for storing which registers are defined
/// and which stack offsets are stored in a branch.
struct BranchInfo {

  std::vector<std::pair<unsigned, const MachineInstr &>> RegDefs;
  std::vector<std::pair<signed, const MachineInstr &>> StackStores;

  /// Merges the branch info with the current branch info by only taking
  /// the register definitions and stack stores that occur in both instances.
  BranchInfo merge(BranchInfo BI) {
    BranchInfo MergedBranchInfo;

    // Merge register definitions
    for (auto OtherDef : BI.RegDefs) {
      auto It = std::find_if(
          RegDefs.begin(), RegDefs.end(),
          [&OtherDef](std::pair<unsigned, const MachineInstr &> RegDef) {
            return RegDef.first == OtherDef.first;
          });
      if (It != RegDefs.end()) {
        MergedBranchInfo.RegDefs.push_back(OtherDef);
      }
    }

    // Merge stack stores
    for (auto OtherStore : BI.StackStores) {
      auto It = std::find_if(
          StackStores.begin(), StackStores.end(),
          [&OtherStore](std::pair<signed, const MachineInstr &> StackStore) {
            return StackStore.first == OtherStore.first;
          });
      if (It != StackStores.end()) {
        MergedBranchInfo.StackStores.push_back(OtherStore);
      }
    }

    return MergedBranchInfo;
  }
};

/// Constructs the branch info for the machine basic block.
BranchInfo constructBranchInfo(const MachineBasicBlock *MBB);

} // namespace RISCV64MachineInstructionUtils
} // namespace mctoll
} // namespace llvm

#endif // LLVM_TOOLS_LLVM_MCTOLL_RISCV_RISCV64_RISCV64MACHINEINSTRUCTIONUTILS_H
