//===-- RISCV64ValueTracker.cpp ---------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains the definition of RISCV64ValueTracker class for use by
// llvm-mctoll.
//
//===----------------------------------------------------------------------===//

#include "RISCV64ValueTracker.h"
#include "RISCV64/RISCV64MachineInstructionRaiser.h"
#include "llvm/ADT/Twine.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/raw_ostream.h"
#include <string>

using namespace llvm;
using namespace mctoll;
using namespace RISCV64MachineInstructionRaiserUtils;

RISCV64ValueTracker::RISCV64ValueTracker(RISCV64MachineInstructionRaiser *MIR)
    : MIR(MIR), MF(MIR->getMF()), C(MIR->getMF().getFunction().getContext()) {}

Value *RISCV64ValueTracker::getRegValue(int MBBNo, unsigned int RegNo) {
  Value *RegVal = MBBRegValues[MBBNo][RegNo];

  // Register not defined locally, look at predecessors for definitions
  if (RegVal == nullptr) {
    const MachineBasicBlock *MBB = MF.getBlockNumbered(MBBNo);
    auto Defs = getDefinitions(RegNo, MBB);

    // If all predecessors define the register, promote to stack slot
    if (Defs.size() == MBB->pred_size() && MBB->pred_size() != 0) {
      BasicBlock *EntryBB = MIR->getBasicBlock(0);
      IRBuilder<> EntryBuilder(EntryBB);

      // Create alloca for stack promotion and insert at end of entry block
      Type *Ty = getStackSlotType(Defs);
      std::string Name = getRegName(RegNo) + "_stack_slot";
      AllocaInst *Alloca = EntryBuilder.CreateAlloca(Ty, nullptr, Name);

      // Let all definitions store to created stack slot
      for (RegisterDefinition Def : Defs) {
        BasicBlock *PredBB = MIR->getBasicBlock(Def.MBBNo);
        IRBuilder<> PredBuilder(PredBB);
        PredBuilder.CreateStore(Def.Val, Alloca);
      }

      // Load from created stack slot
      BasicBlock *BB = MIR->getBasicBlock(MBBNo);
      IRBuilder<> Builder(BB);
      RegVal = Builder.CreateLoad(Ty, Alloca);
      setRegValue(MBBNo, RegNo, RegVal);
    }
    // Exactly one predecessor defines this register, no stack
    // promotion needed, return value defined by that predecessor
    else if (Defs.size() == 1) {
      RegVal = Defs.at(0).Val;
    } else if (Defs.size() != 0) {
      errs() << "unexpected number of definitions by predecessors\n";
    }
  }

  return RegVal;
}

void RISCV64ValueTracker::setRegValue(int MBBNo, unsigned int RegNo,
                                      Value *Val) {
  MBBRegValues[MBBNo][RegNo] = Val;
}

Value *RISCV64ValueTracker::getStackSlot(int StackOffset, Type *AllocaTy) {
  if (AllocaTy == nullptr) {
    AllocaTy = Type::getInt64Ty(C);
  }

  if (StackValues[StackOffset] == nullptr) {
    BasicBlock *EntryBB = MIR->getBasicBlock(0);
    IRBuilder<> EntryBuilder(EntryBB);

    AllocaInst *Alloca = EntryBuilder.CreateAlloca(AllocaTy);
    StackValues[StackOffset] = Alloca;
  }

  return StackValues[StackOffset];
}

MachineBasicBlock::const_reverse_instr_iterator
RISCV64ValueTracker::getFinalDefinition(unsigned int RegNo,
                                        const MachineBasicBlock *MBB) {
  for (auto It = MBB->instr_rbegin(); It != MBB->instr_rend(); ++It) {
    if (It->definesRegister(RegNo)) {
      return It;
    }
  }
  return MBB->instr_rend();
}

std::vector<RegisterDefinition>
RISCV64ValueTracker::getDefinitions(unsigned int RegNo,
                                    const MachineBasicBlock *MBB) {
  std::vector<RegisterDefinition> Definitions;
  for (const MachineBasicBlock *PredMBB : MBB->predecessors()) {
    auto It = getFinalDefinition(RegNo, PredMBB);
    if (It != PredMBB->instr_rend()) {
      int MBBNo = PredMBB->getNumber();
      const MachineInstr &MI = *It;
      Value *Val = MBBRegValues[MBBNo][RegNo];

      RegisterDefinition Definition = {MBBNo, MI, Val};

      Definitions.push_back(Definition);
    }
  }
  return Definitions;
}

Type *RISCV64ValueTracker::getStackSlotType(
    const std::vector<RegisterDefinition> &Defs) {
  auto Begin = Defs.begin();
  Type *Ty = Begin->Val->getType();

  for (auto It = ++Begin; It != Defs.end(); ++It) {
    Type *ValTy = It->Val->getType();

    // Determine widest integer type of definitions
    if (ValTy->isIntegerTy() && Ty->isIntegerTy() &&
        ValTy->getIntegerBitWidth() > Ty->getIntegerBitWidth()) {
      Ty = ValTy;
    } else if (ValTy->isIntegerTy() && Ty->isIntegerTy() &&
               ValTy->getIntegerBitWidth() < Ty->getIntegerBitWidth()) {
      ValTy = Ty;
    }

    assert(Ty == ValTy && "not all branch definitions have the same type!");
  }

  return Ty;
}
