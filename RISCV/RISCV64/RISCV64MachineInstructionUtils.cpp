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
#include <algorithm>
#include <cassert>

#define DEBUG_TYPE "mctoll"

using namespace llvm;
using namespace llvm::mctoll;
using namespace llvm::mctoll::RISCV64MachineInstructionUtils;

Type *RISCV64MachineInstructionUtils::getDefaultType(LLVMContext &C,
                                                     const MachineInstr &MI) {
  if (MI.getOpcode() == RISCV::LD || MI.getOpcode() == RISCV::C_LD ||
      MI.getOpcode() == RISCV::SD || MI.getOpcode() == RISCV::C_SD) {
    return getDefaultPtrType(C);
  }
  return getDefaultIntType(C);
}

IntegerType *RISCV64MachineInstructionUtils::getDefaultIntType(LLVMContext &C) {
  return Type::getInt32Ty(C);
}

PointerType *RISCV64MachineInstructionUtils::getDefaultPtrType(LLVMContext &C) {
  return Type::getInt64PtrTy(C);
}

ConstantInt *RISCV64MachineInstructionUtils::toConstantInt(LLVMContext &C,
                                                           uint64_t V) {
  return ConstantInt::get(getDefaultIntType(C), V);
}

ConstantInt *RISCV64MachineInstructionUtils::toGEPIndex(LLVMContext &C,
                                                        uint64_t Offset) {
  IntegerType *Ty = getDefaultIntType(C);
  unsigned Width = Ty->getBitWidth() / 8;

  // Make sure offset is a multiple of Width
  if (Offset & (Width - 1)) {
    errs() << "offset " << Offset << " is not aligned to " << Width
           << " bytes\n";
    exit(EXIT_FAILURE);
  }

  uint64_t V = Offset / Width;

  return toConstantInt(C, V);
}

InstructionType
RISCV64MachineInstructionUtils::getInstructionType(unsigned Op) {
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

BinaryOps RISCV64MachineInstructionUtils::toBinaryOperation(unsigned Op) {
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

Predicate RISCV64MachineInstructionUtils::toPredicate(unsigned Op) {
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

bool RISCV64MachineInstructionUtils::isAddI(unsigned Op) {
  return Op == RISCV::ADDI || Op == RISCV::ADDIW || Op == RISCV::C_ADDI ||
         Op == RISCV::C_ADDIW || Op == RISCV::C_ADDI4SPN ||
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
    return isAddI(MI.getOpcode()) && MI.getOperand(0).isReg() &&
           MI.getOperand(0).getReg() == RISCV::X2 && MI.getOperand(1).isReg() &&
           MI.getOperand(1).getReg() == RISCV::X2 && MI.getOperand(2).isImm() &&
           MI.getOperand(2).getImm() < 0;
  };
  auto IsAdjustFramePointerInstruction = [](const MachineInstr &MI) {
    return isAddI(MI.getOpcode()) && MI.getOperand(0).isReg() &&
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
    return isAddI(MI.getOpcode()) && MI.getOperand(0).isReg() &&
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

BranchInfo RISCV64MachineInstructionUtils::constructBranchInfo(
    const MachineBasicBlock *MBB) {
  BranchInfo BI;
  for (const MachineInstr &MI : MBB->instrs()) {
    InstructionType Type = getInstructionType(MI.getOpcode());
    // Check if instruction stores to stack
    if (Type == InstructionType::STORE) {
      const MachineOperand &MOp1 = MI.getOperand(0);
      const MachineOperand &MOp2 = MI.getOperand(1);
      const MachineOperand &MOp3 = MI.getOperand(2);
      assert(MOp1.isReg() && MOp2.isReg() && MOp3.isImm());
      if (MOp2.getReg() == RISCV::X8) {
        BI.StackStores.emplace_back(MOp3.getImm(), MI);
      }
    }
    // For now, only check if instruction defines register x15/a5
    else if (MI.definesRegister(RISCV::X15)) {
      BI.RegisterDefinitions.emplace_back(RISCV::X15, MI);
    }
  }
  return BI;
}
