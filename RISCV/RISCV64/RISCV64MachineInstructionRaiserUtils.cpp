//===-- RISCV64MachineInstructionRaiserUtils.cpp ----------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains the definitions of multiple utility functions use for
// discovering the function prototypes and raising the machine functions for
// use by llvm-mctoll.
//
//===----------------------------------------------------------------------===//

#include "RISCV64MachineInstructionRaiserUtils.h"
#include "MCTargetDesc/RISCVMCTargetDesc.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Type.h"
#include <algorithm>

#define DEBUG_TYPE "mctoll"

using namespace llvm;
using namespace mctoll;
using namespace RISCV;
using namespace RISCV64MachineInstructionRaiserUtils;

std::string
RISCV64MachineInstructionRaiserUtils::getRegName(unsigned int RegNo) {
  return "x" + std::to_string(RegNo - X0);
}

bool RISCV64MachineInstructionRaiserUtils::isArgReg(unsigned int RegNo) {
  return RegNo >= X10 && RegNo <= X17;
}

Type *
RISCV64MachineInstructionRaiserUtils::getDefaultType(LLVMContext &C,
                                                     const MachineInstr &MI) {
  if (getAlign(MI.getOpcode()) == DoubleWordAlign) {
    return Type::getInt64Ty(C);
  }
  return getDefaultIntType(C);
}

IntegerType *
RISCV64MachineInstructionRaiserUtils::getDefaultIntType(LLVMContext &C) {
  return Type::getInt32Ty(C);
}

PointerType *
RISCV64MachineInstructionRaiserUtils::getDefaultPtrType(LLVMContext &C) {
  return Type::getInt64PtrTy(C);
}

uint64_t RISCV64MachineInstructionRaiserUtils::getAlign(unsigned int Op) {
  switch (Op) {
  case SD:
  case C_SD:
  case LD:
  case C_LD:
    return DoubleWordAlign;
  default:
    return SingleWordAlign;
  }
}

ConstantInt *RISCV64MachineInstructionRaiserUtils::toGEPIndex(LLVMContext &C,
                                                              uint64_t Offset,
                                                              uint64_t Align) {
  assert(!(Offset & (Align - 1)) &&
         "GEP index offset is not correctly aligned");

  uint64_t V = Offset / Align;

  return ConstantInt::get(getDefaultIntType(C), V);
}

bool RISCV64MachineInstructionRaiserUtils::isAddI(unsigned int Op) {
  return Op == ADDI || Op == ADDIW || Op == C_ADDI || Op == C_ADDIW ||
         Op == C_ADDI4SPN || Op == C_ADDI16SP;
}

bool RISCV64MachineInstructionRaiserUtils::isLoad(unsigned int Op) {
  return Op == LB || Op == LBU || Op == LH || Op == LHU || Op == LW ||
         Op == LWU || Op == LD || Op == C_LW || Op == C_LD || Op == C_LDSP;
}

bool RISCV64MachineInstructionRaiserUtils::isStore(unsigned int Op) {
  return Op == SB || Op == SH || Op == SW || Op == SD || Op == C_SW ||
         Op == C_SD || Op == C_SDSP;
}

BinaryOps
RISCV64MachineInstructionRaiserUtils::toBinaryOperation(unsigned int Op) {
  switch (Op) {
  case ADD:
  case ADDW:
  case C_ADD:
  case C_ADDW:
  case ADDI:
  case ADDIW:
  case C_ADDI:
  case C_ADDIW:
  case C_ADDI4SPN:
  case C_ADDI16SP:
    return BinaryOps::Add;
  case SUB:
  case SUBW:
  case C_SUB:
  case C_SUBW:
    return BinaryOps::Sub;
  case MUL:
  case MULH:
  case MULHU:
  case MULHSU:
  case MULW:
    return BinaryOps::Mul;
  case DIV:
  case DIVW:
    return BinaryOps::SDiv;
  case DIVU:
  case DIVUW:
    return BinaryOps::UDiv;
  case SLL:
  case SLLW:
  case SLLI:
  case SLLIW:
  case C_SLLI:
    return BinaryOps::Shl;
  case SRL:
  case SRLW:
  case SRLI:
  case SRLIW:
  case C_SRLI:
    return BinaryOps::LShr;
  case SRA:
  case SRAW:
  case SRAI:
  case SRAIW:
  case C_SRAI:
    return BinaryOps::AShr;
  case AND:
  case ANDI:
  case C_AND:
  case C_ANDI:
    return BinaryOps::And;
  case OR:
  case ORI:
  case C_OR:
    return BinaryOps::Or;
  case XOR:
  case XORI:
  case C_XOR:
    return BinaryOps::Xor;
  default:
    return BinaryOps::BinaryOpsEnd;
  }
}

Predicate RISCV64MachineInstructionRaiserUtils::toPredicate(unsigned int Op) {
  switch (Op) {
  case BEQ:
  case C_BEQZ:
    return Predicate::ICMP_EQ;
  case BNE:
  case C_BNEZ:
    return Predicate::ICMP_NE;
  case BGE:
    return Predicate::ICMP_SGE;
  case BGEU:
    return Predicate::ICMP_UGE;
  case BLT:
    return Predicate::ICMP_SLT;
  case BLTU:
    return Predicate::ICMP_ULT;
  default:
    return Predicate::BAD_ICMP_PREDICATE;
  }
}

bool RISCV64MachineInstructionRaiserUtils::isPrologInstruction(
    const MachineInstr &MI) {
  auto IsDecreaseStackPointerInstruction = [](const MachineInstr &MI) {
    return isAddI(MI.getOpcode()) && MI.getOperand(0).isReg() &&
           MI.getOperand(0).getReg() == X2 && MI.getOperand(1).isReg() &&
           MI.getOperand(1).getReg() == X2 && MI.getOperand(2).isImm() &&
           MI.getOperand(2).getImm() < 0;
  };
  auto IsStoreReturnAddress = [](const MachineInstr &MI) {
    return isStore(MI.getOpcode()) && MI.getOperand(0).isReg() &&
           MI.getOperand(0).getReg() == X1;
  };
  auto IsStoreStackPointer = [](const MachineInstr &MI) {
    return isStore(MI.getOpcode()) && MI.getOperand(0).isReg() &&
           MI.getOperand(0).getReg() == X8;
  };
  auto IsSetFramePointer = [](const MachineInstr &MI) {
    return toBinaryOperation(MI.getOpcode()) != BinaryOps::BinaryOpsEnd &&
           MI.getOperand(0).getReg() == X8 && MI.getOperand(1).getReg() == X2;
  };
  return IsStoreReturnAddress(MI) || IsStoreStackPointer(MI) ||
         IsSetFramePointer(MI);
}

bool RISCV64MachineInstructionRaiserUtils::isEpilogInstruction(
    const MachineInstr &MI) {
  auto IsIncreaseStackPointerInstruction = [](const MachineInstr &MI) {
    return isAddI(MI.getOpcode()) && MI.getOperand(0).isReg() &&
           MI.getOperand(0).getReg() == X2 && MI.getOperand(1).isReg() &&
           MI.getOperand(1).getReg() == X2 && MI.getOperand(2).isImm() &&
           MI.getOperand(2).getImm() > 0;
  };
  auto IsLoadReturnAddress = [](const MachineInstr &MI) {
    return isLoad(MI.getOpcode()) && MI.getOperand(0).isReg() &&
           MI.getOperand(0).getReg() == X1;
  };
  auto IsLoadStackPointer = [](const MachineInstr &MI) {
    return isLoad(MI.getOpcode()) && MI.getOperand(0).isReg() &&
           MI.getOperand(0).getReg() == X8;
  };
  return IsIncreaseStackPointerInstruction(MI) || IsLoadReturnAddress(MI) ||
         IsLoadStackPointer(MI) || MI.getOpcode() == C_LDSP ||
         MI.getOpcode() == C_ADDI16SP;
}

bool RISCV64MachineInstructionRaiserUtils::isRegisterDefined(
    unsigned int RegNo, MachineBasicBlock::const_instr_iterator Begin,
    MachineBasicBlock::const_instr_iterator End) {
  auto Pred = [&RegNo](const MachineInstr &MI) {
    return MI.definesRegister(RegNo);
  };
  return std::any_of(Begin, End, Pred);
}

MachineBasicBlock::const_reverse_instr_iterator
RISCV64MachineInstructionRaiserUtils::findInstructionByOpcode(
    unsigned int Op, MachineBasicBlock::const_reverse_instr_iterator Begin,
    MachineBasicBlock::const_reverse_instr_iterator End) {
  auto Pred = [&Op](const MachineInstr &MI) { return MI.getOpcode() == Op; };
  return std::find_if(Begin, End, Pred);
}

MachineBasicBlock::const_reverse_instr_iterator
RISCV64MachineInstructionRaiserUtils::findInstructionByRegNo(
    unsigned int RegNo, MachineBasicBlock::const_reverse_instr_iterator Begin,
    MachineBasicBlock::const_reverse_instr_iterator End) {
  auto Pred = [&RegNo](const MachineInstr &MI) {
    return MI.definesRegister(RegNo);
  };
  return std::find_if(Begin, End, Pred);
}
