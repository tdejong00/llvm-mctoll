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
#include "llvm/IR/Instructions.h"

using namespace llvm;
using namespace llvm::mctoll;

  return MBBRegValues[MBBNo][RegNo];
RISCV64ValueTracker::RISCV64ValueTracker(RISCV64MachineInstructionRaiser *MIR)
    : MIR(MIR), MF(MIR->getMF()), C(MIR->getMF().getFunction().getContext()) {}

Value *RISCV64ValueTracker::getRegValue(int MBBNo, unsigned int RegNo) {
}

void RISCV64ValueTracker::setRegValue(int MBBNo, unsigned int RegNo,
                                      Value *Val) {
  MBBRegValues[MBBNo][RegNo] = Val;
}

Value *RISCV64ValueTracker::getStackValue(int StackOffset) {
  return StackValues[StackOffset];
}

void RISCV64ValueTracker::setStackValue(int StackOffset, Value *Val) {
  StackValues[StackOffset] = Val;
}
