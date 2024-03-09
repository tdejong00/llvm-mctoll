//===-- RISCV64MachineInstructionRaiser.cpp ---------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains the implementation of RISCV64ModuleRaiser class
// for use by llvm-mctoll.
//
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/RISCVMCTargetDesc.h"
#include "RISCVSubtarget.h"
#include "RISCV64MachineInstructionRaiser.h"
#include "RISCVModuleRaiser.h"
#include "Raiser/MachineFunctionRaiser.h"
#include "llvm/CodeGen/LivePhysRegs.h"
#include "llvm/CodeGen/LoopTraversal.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineJumpTableInfo.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/RegisterPressure.h"
#include "llvm/CodeGen/TargetRegisterInfo.h"
#include "llvm/CodeGen/TargetSubtargetInfo.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"
#include <algorithm>
#include <cstddef>

#define DEBUG_TYPE "mctoll"

using namespace llvm;
using namespace llvm::mctoll;

// NOTE : The following RISCV64ModuleRaiser class function is defined here as
// they reference MachineFunctionRaiser class that has a forward declaration
// in ModuleRaiser.h.

// Create a new MachineFunctionRaiser object and add it to the list of
// MachineFunction raiser objects of this module.
MachineFunctionRaiser *RISCV64ModuleRaiser::CreateAndAddMachineFunctionRaiser(
    Function *F, const ModuleRaiser *MR, uint64_t Start, uint64_t End) {
  MachineFunctionRaiser *MFR = new MachineFunctionRaiser(
      *M, MR->getMachineModuleInfo()->getOrCreateMachineFunction(*F), MR, Start,
      End);

  MFR->setMachineInstrRaiser(new RISCV64MachineInstructionRaiser(
      MFR->getMachineFunction(), MR, MFR->getMCInstRaiser()));

  MFRaiserVector.push_back(MFR);
  return MFR;
}

RISCV64MachineInstructionRaiser::RISCV64MachineInstructionRaiser(
    MachineFunction &MF, const ModuleRaiser *MR, MCInstRaiser *MCIR)
    : MachineInstructionRaiser(MF, MR, MCIR) {}

bool RISCV64MachineInstructionRaiser::raise() { 
  errs() << "Not yet implemented: RISCV64MachineInstructionRaiser::raise\n";
  return false; 
}

FunctionType *RISCV64MachineInstructionRaiser::getRaisedFunctionPrototype() {
  errs() << "Not yet implemented: RISCV64MachineInstructionRaiser::getRaisedFunctionPrototype\n";
  return nullptr;
}

int RISCV64MachineInstructionRaiser::getArgumentNumber(unsigned PReg) {
  errs() << "Not yet implemented: RISCV64MachineInstructionRaiser::getArgumentNumber\n";
  return 0;
}

Value *RISCV64MachineInstructionRaiser::getRegOrArgValue(unsigned PReg, int MBBNo) {
  errs() << "Not yet implemented: RISCV64MachineInstructionRaiser::getRegOrArgValue\n";
  return nullptr;
}

bool RISCV64MachineInstructionRaiser::buildFuncArgTypeVector(const std::set<MCPhysReg> &, std::vector<Type *> &) {
  errs() << "Not yet implemented: RISCV64MachineInstructionRaiser::buildFuncArgTypeVector\n";
  return false;
}
