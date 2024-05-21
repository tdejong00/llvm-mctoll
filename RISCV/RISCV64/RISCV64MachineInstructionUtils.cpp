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
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/Debug.h"
#include <algorithm>

#define DEBUG_TYPE "mctoll"

using namespace llvm;
using namespace llvm::mctoll;
using namespace llvm::mctoll::RISCV64MachineInstructionUtils;

std::string RISCV64MachineInstructionUtils::getRegName(unsigned int RegNo) {
  return "x" + std::to_string(RegNo - RISCV::X0);
}

bool RISCV64MachineInstructionUtils::isArgReg(unsigned int RegNo) {
  return RegNo >= RISCV::X10 && RegNo <= RISCV::X17;
}

Type *RISCV64MachineInstructionUtils::getDefaultType(LLVMContext &C,
                                                     const MachineInstr &MI) {
  if (MI.getOpcode() == RISCV::LD || MI.getOpcode() == RISCV::C_LD ||
      MI.getOpcode() == RISCV::SD || MI.getOpcode() == RISCV::C_SD) {
    return Type::getInt64Ty(C);
  }
  return getDefaultIntType(C);
}

IntegerType *RISCV64MachineInstructionUtils::getDefaultIntType(LLVMContext &C) {
  return Type::getInt32Ty(C);
}

PointerType *RISCV64MachineInstructionUtils::getDefaultPtrType(LLVMContext &C) {
  return Type::getInt64PtrTy(C);
}

ConstantInt *RISCV64MachineInstructionUtils::toGEPIndex(LLVMContext &C,
                                                        uint64_t Offset,
                                                        IntegerType *Ty) {
  unsigned int Width = Ty->getBitWidth() / 8;

  // Make sure offset is a multiple of Width
  if (Offset & (Width - 1)) {
    errs() << "offset " << Offset << " is not aligned to " << Width
           << " bytes\n";
    exit(EXIT_FAILURE);
  }

  uint64_t V = Offset / Width;

  return ConstantInt::get(getDefaultIntType(C), V);
}

InstructionType
RISCV64MachineInstructionUtils::getInstructionType(unsigned int Op) {
  switch (Op) {
  case RISCV::C_NOP:
    return InstructionType::NOP;
  case RISCV::C_LI:
  case RISCV::C_MV:
    return InstructionType::MOVE;
  case RISCV::LB:
  case RISCV::LBU:
  case RISCV::LH:
  case RISCV::LHU:
  case RISCV::LW:
  case RISCV::LWU:
  case RISCV::LD:
  case RISCV::C_LW:
  case RISCV::C_LD:
    return InstructionType::LOAD;
  case RISCV::SB:
  case RISCV::SH:
  case RISCV::SW:
  case RISCV::SD:
  case RISCV::C_SW:
  case RISCV::C_SD:
    return InstructionType::STORE;
  case RISCV::AUIPC:
    return InstructionType::GLOBAL;
  case RISCV::JAL:
    return InstructionType::CALL;
  case RISCV::C_JR:
    return InstructionType::RETURN;
  case RISCV::C_J:
    return InstructionType::UNCONDITIONAL_BRANCH;
  default:
    if (toBinaryOperation(Op) != BinaryOps::BinaryOpsEnd) {
      return InstructionType::BINOP;
    }
    if (toPredicate(Op) != Predicate::BAD_ICMP_PREDICATE) {
      return InstructionType::CONDITIONAL_BRANCH;
    }
    return InstructionType::UNKNOWN;
  }
}

BinaryOps RISCV64MachineInstructionUtils::toBinaryOperation(unsigned int Op) {
  switch (Op) {
  case RISCV::ADD:
  case RISCV::ADDW:
  case RISCV::C_ADD:
  case RISCV::C_ADDW:
  case RISCV::ADDI:
  case RISCV::ADDIW:
  case RISCV::C_ADDI:
  case RISCV::C_ADDIW:
  case RISCV::C_ADDI4SPN:
  case RISCV::C_ADDI16SP:
    return BinaryOps::Add;
  case RISCV::SUB:
  case RISCV::SUBW:
  case RISCV::C_SUB:
  case RISCV::C_SUBW:
    return BinaryOps::Sub;
  case RISCV::MUL:
  case RISCV::MULH:
  case RISCV::MULHU:
  case RISCV::MULHSU:
  case RISCV::MULW:
    return BinaryOps::Mul;
  case RISCV::DIV:
  case RISCV::DIVW:
    return BinaryOps::SDiv;
  case RISCV::DIVU:
  case RISCV::DIVUW:
    return BinaryOps::UDiv;
  case RISCV::SLL:
  case RISCV::SLLW:
  case RISCV::SLLI:
  case RISCV::SLLIW:
  case RISCV::C_SLLI:
    return BinaryOps::Shl;
  case RISCV::SRL:
  case RISCV::SRLW:
  case RISCV::SRLI:
  case RISCV::SRLIW:
  case RISCV::C_SRLI:
    return BinaryOps::LShr;
  case RISCV::SRA:
  case RISCV::SRAW:
  case RISCV::SRAI:
  case RISCV::SRAIW:
  case RISCV::C_SRAI:
    return BinaryOps::AShr;
  case RISCV::AND:
  case RISCV::ANDI:
  case RISCV::C_AND:
  case RISCV::C_ANDI:
    return BinaryOps::And;
  case RISCV::OR:
  case RISCV::ORI:
  case RISCV::C_OR:
    return BinaryOps::Or;
  case RISCV::XOR:
  case RISCV::XORI:
  case RISCV::C_XOR:
    return BinaryOps::Xor;
  default:
    return BinaryOps::BinaryOpsEnd;
  }
}

Predicate RISCV64MachineInstructionUtils::toPredicate(unsigned int Op) {
  switch (Op) {
  case RISCV::BEQ:
  case RISCV::C_BEQZ:
    return Predicate::ICMP_EQ;
  case RISCV::BNE:
  case RISCV::C_BNEZ:
    return Predicate::ICMP_NE;
  case RISCV::BGE:
    return Predicate::ICMP_SGE;
  case RISCV::BGEU:
    return Predicate::ICMP_UGE;
  case RISCV::BLT:
    return Predicate::ICMP_SLT;
  case RISCV::BLTU:
    return Predicate::ICMP_ULT;
  default:
    return Predicate::BAD_ICMP_PREDICATE;
  }
}

bool RISCV64MachineInstructionUtils::isAddI(unsigned int Op) {
  return Op == RISCV::ADDI || Op == RISCV::ADDIW || Op == RISCV::C_ADDI ||
         Op == RISCV::C_ADDIW || Op == RISCV::C_ADDI4SPN ||
         Op == RISCV::C_ADDI16SP;
}

bool RISCV64MachineInstructionUtils::isPrologInstruction(
    const MachineInstr &MI) {
  auto IsDecreaseStackPointerInstruction = [](const MachineInstr &MI) {
    return isAddI(MI.getOpcode()) && MI.getOperand(0).isReg() &&
           MI.getOperand(0).getReg() == RISCV::X2 && MI.getOperand(1).isReg() &&
           MI.getOperand(1).getReg() == RISCV::X2 && MI.getOperand(2).isImm() &&
           MI.getOperand(2).getImm() < 0;
  };
  auto IsStoreReturnAddress = [](const MachineInstr &MI) {
    return getInstructionType(MI.getOpcode()) == InstructionType::STORE &&
           MI.getOperand(0).isReg() && MI.getOperand(0).getReg() == RISCV::X1;
  };
  auto IsStoreStackPointer = [](const MachineInstr &MI) {
    return getInstructionType(MI.getOpcode()) == InstructionType::STORE &&
           MI.getOperand(0).isReg() && MI.getOperand(0).getReg() == RISCV::X8;
  };
  return IsDecreaseStackPointerInstruction(MI) || IsStoreReturnAddress(MI) ||
         IsStoreStackPointer(MI) || MI.getOpcode() == RISCV::C_SDSP ||
         MI.getOpcode() == RISCV::C_ADDI4SPN;
}

bool RISCV64MachineInstructionUtils::isEpilogInstruction(
    const MachineInstr &MI) {
  auto IsIncreaseStackPointerInstruction = [](const MachineInstr &MI) {
    return isAddI(MI.getOpcode()) && MI.getOperand(0).isReg() &&
           MI.getOperand(0).getReg() == RISCV::X2 && MI.getOperand(1).isReg() &&
           MI.getOperand(1).getReg() == RISCV::X2 && MI.getOperand(2).isImm() &&
           MI.getOperand(2).getImm() > 0;
  };
  auto IsLoadReturnAddress = [](const MachineInstr &MI) {
    return getInstructionType(MI.getOpcode()) == InstructionType::LOAD &&
           MI.getOperand(0).isReg() && MI.getOperand(0).getReg() == RISCV::X1;
  };
  auto IsLoadStackPointer = [](const MachineInstr &MI) {
    return getInstructionType(MI.getOpcode()) == InstructionType::LOAD &&
           MI.getOperand(0).isReg() && MI.getOperand(0).getReg() == RISCV::X8;
  };
  return IsIncreaseStackPointerInstruction(MI) || IsLoadReturnAddress(MI) ||
         IsLoadStackPointer(MI) || MI.getOpcode() == RISCV::C_LDSP ||
         MI.getOpcode() == RISCV::C_ADDI16SP;
}

bool RISCV64MachineInstructionUtils::isRegisterDefined(
    unsigned int RegNo, MachineBasicBlock::const_instr_iterator Begin,
    MachineBasicBlock::const_instr_iterator End) {
  auto Pred = [&RegNo](const MachineInstr &MI) {
    return MI.definesRegister(RegNo);
  };
  return std::any_of(Begin, End, Pred);
}

MachineBasicBlock::const_reverse_instr_iterator
RISCV64MachineInstructionUtils::findInstructionByOpcode(
    unsigned int Op, MachineBasicBlock::const_reverse_instr_iterator Begin,
    MachineBasicBlock::const_reverse_instr_iterator End) {
  auto Pred = [&Op](const MachineInstr &MI) { return MI.getOpcode() == Op; };
  return std::find_if(Begin, End, Pred);
}

MachineBasicBlock::const_reverse_instr_iterator
RISCV64MachineInstructionUtils::findInstructionByRegNo(
    unsigned int RegNo, MachineBasicBlock::const_reverse_instr_iterator Begin,
    MachineBasicBlock::const_reverse_instr_iterator End) {
  auto Pred = [&RegNo](const MachineInstr &MI) {
    return MI.definesRegister(RegNo);
  };
  return std::find_if(Begin, End, Pred);
}
