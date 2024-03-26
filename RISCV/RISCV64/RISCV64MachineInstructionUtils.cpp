//===-- RISCV64MachineInstructionUtils.cpp ----------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains the definition of multiple utility functions regarding
// machine instructions and machine basic blocks for use by llvm-mctoll.
//
//===----------------------------------------------------------------------===//

#include "RISCV64MachineInstructionUtils.h"
#include "MCTargetDesc/RISCVMCTargetDesc.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Type.h"
#include <algorithm>

#define DEBUG_TYPE "mctoll"

using namespace llvm;
using namespace llvm::mctoll;

IntegerType *
RISCV64MachineInstructionUtils::getDefaultIntType(MachineFunction &MF) {
  LLVMContext &C = MF.getFunction().getContext();
  return Type::getInt32Ty(C);
}

PointerType *
RISCV64MachineInstructionUtils::getDefaultPtrType(MachineFunction &MF) {
  LLVMContext &C = MF.getFunction().getContext();
  return Type::getInt32PtrTy(C);
}

inline bool isAddInstruction(unsigned Op) {
  return Op == RISCV::C_ADDI || Op == RISCV::C_ADDI4SPN ||
         Op == RISCV::C_ADDI16SP;
}

bool RISCV64MachineInstructionUtils::isPrologInstruction(
    const MachineInstr &MI) {
  // The following instructions are the prolog instructions we want to skip:
  // - adjusting stack pointer (build up stack)
  // - adjusting frame pointer
  // - storing stack pointer
  // - storing return address

  auto IsAdjustStackPointerInstruction = [](const MachineInstr &MI) {
    return isAddInstruction(MI.getOpcode()) && MI.getOperand(0).isReg() &&
           MI.getOperand(0).getReg() == RISCV::X2 && MI.getOperand(1).isReg() &&
           MI.getOperand(1).getReg() == RISCV::X2 && MI.getOperand(2).isImm() &&
           MI.getOperand(2).getImm() < 0;
  };
  auto IsAdjustFramePointerInstruction = [](const MachineInstr &MI) {
    return isAddInstruction(MI.getOpcode()) && MI.getOperand(0).isReg() &&
           MI.getOperand(0).getReg() == RISCV::X8 && MI.getOperand(1).isReg() &&
           MI.getOperand(1).getReg() == RISCV::X2 && MI.getOperand(2).isImm();
  };
  auto IsStoreFramePointerInstruction = [](const MachineInstr &MI) {
    return MI.getOpcode() == RISCV::C_SDSP && MI.getOperand(0).isReg() &&
           MI.getOperand(0).getReg() == RISCV::X8 && MI.getOperand(1).isReg() &&
           MI.getOperand(1).getReg() == RISCV::X2 && MI.getOperand(2).isImm();
  };
  auto IsStoreReturnAddressInstruction = [](const MachineInstr &MI) {
    return MI.getOpcode() == RISCV::C_SDSP && MI.getOperand(0).isReg() &&
           MI.getOperand(0).getReg() == RISCV::X1 && MI.getOperand(1).isReg() &&
           MI.getOperand(1).getReg() == RISCV::X2 && MI.getOperand(2).isImm();
  };
  return IsAdjustStackPointerInstruction(MI) ||
         IsAdjustFramePointerInstruction(MI) ||
         IsStoreFramePointerInstruction(MI) ||
         IsStoreReturnAddressInstruction(MI);
}

bool RISCV64MachineInstructionUtils::isEpilogInstruction(
    const MachineInstr &MI) {
  // The following instructions are the epilog instructions we want to skip:
  // - adjusting stack pointer (take down stack)
  // - loading the frame pointer
  // - loading the return address
  auto IsAdjustStackPointerInstruction = [](const MachineInstr &MI) {
    return isAddInstruction(MI.getOpcode()) && MI.getOperand(0).isReg() &&
           MI.getOperand(0).getReg() == RISCV::X2 && MI.getOperand(1).isReg() &&
           MI.getOperand(1).getReg() == RISCV::X2 && MI.getOperand(2).isImm() &&
           MI.getOperand(2).getImm() > 0;
  };
  auto IsLoadFramePointerInstruction = [](const MachineInstr &MI) {
    return MI.getOpcode() == RISCV::C_LDSP && MI.getOperand(0).isReg() &&
           MI.getOperand(0).getReg() == RISCV::X8 && MI.getOperand(1).isReg() &&
           MI.getOperand(1).getReg() == RISCV::X2 && MI.getOperand(2).isImm();
  };
  auto IsLoadReturnAddressInstruction = [](const MachineInstr &MI) {
    return MI.getOpcode() == RISCV::C_LDSP && MI.getOperand(0).isReg() &&
           MI.getOperand(0).getReg() == RISCV::X1 && MI.getOperand(1).isReg() &&
           MI.getOperand(1).getReg() == RISCV::X2 && MI.getOperand(2).isImm();
  };
  return IsAdjustStackPointerInstruction(MI) ||
         IsLoadFramePointerInstruction(MI) ||
         IsLoadReturnAddressInstruction(MI);
}

MachineBasicBlock::const_instr_iterator
RISCV64MachineInstructionUtils::skipProlog(const MachineBasicBlock &MBB) {
  auto It = MBB.instr_begin();
  while (It != MBB.instr_end() && isPrologInstruction(*It)) {
    ++It;
  }
  return It;
}

MachineBasicBlock::const_reverse_instr_iterator
RISCV64MachineInstructionUtils::findInstructionByOpcode(
    const MachineBasicBlock &MBB, unsigned int Op,
    MachineBasicBlock::const_reverse_instr_iterator EndIt) {
  auto Pred = [&Op](const MachineInstr &MI) { return MI.getOpcode() == Op; };
  return std::find_if(MBB.instr_rbegin(), EndIt, Pred);
}

MachineBasicBlock::const_reverse_instr_iterator
RISCV64MachineInstructionUtils::findInstructionByRegNo(
    const MachineBasicBlock &MBB, unsigned int RegNo,
    MachineBasicBlock::const_reverse_instr_iterator EndIt) {
  auto Pred = [&RegNo](const MachineInstr &MI) {
    return MI.definesRegister(RegNo);
  };
  return std::find_if(MBB.instr_rbegin(), EndIt, Pred);
}
