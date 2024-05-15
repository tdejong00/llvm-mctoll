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
#include <cstdint>
#include <string>

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

/// Gets the string representation of the register.
std::string getRegName(unsigned int RegNo);

/// Determines whether the register is a argument register, i.e. x10-x17.
bool isArgReg(unsigned int RegNo);

/// Creates a ConstantInt representing a GEP index, based on the given pointer
/// offset (number of bytes). The operands of the GEP instructions represent
/// indices and not number of bytes.
ConstantInt *toGEPIndex(LLVMContext &C, uint64_t Offset);

/// Determines the instruction type of the opcode.
InstructionType getInstructionType(unsigned int Op);

/// Converts the opcode to a binary operation.
BinaryOps toBinaryOperation(unsigned int Op);

/// Converts the opcode to a compare predicate.
Predicate toPredicate(unsigned int Op);

/// Determines whether the opcode is an ADDI instruction.
bool isAddI(unsigned int Op);

/// Determines whether the machine instruction is a part of the prolog.
bool isPrologInstruction(const MachineInstr &MI);

/// Determines whether the machine instruction is a part of the epilog.
bool isEpilogInstruction(const MachineInstr &MI);

/// Determines whether the register is defined by one of the instructions
/// from the first instruction of the machine basic block to the specified
/// machine instruction.
bool isRegisterDefined(unsigned int RegNo,
                       MachineBasicBlock::const_instr_iterator Begin,
                       MachineBasicBlock::const_instr_iterator End);

/// Finds the instruction in the machine basic block which has
/// the given opcode. Only searches up until the given end iterator.
MachineBasicBlock::const_reverse_instr_iterator
findInstructionByOpcode(unsigned int Op,
                        MachineBasicBlock::const_reverse_instr_iterator Begin,
                        MachineBasicBlock::const_reverse_instr_iterator End);

/// Finds the instruction in the machine basic block which defines the
/// given register number. Only searches up until the given end iterator.
MachineBasicBlock::const_reverse_instr_iterator
findInstructionByRegNo(unsigned int RegNo,
                       MachineBasicBlock::const_reverse_instr_iterator Begin,
                       MachineBasicBlock::const_reverse_instr_iterator End);

} // namespace RISCV64MachineInstructionUtils
} // namespace mctoll
} // namespace llvm

#endif // LLVM_TOOLS_LLVM_MCTOLL_RISCV_RISCV64_RISCV64MACHINEINSTRUCTIONUTILS_H
