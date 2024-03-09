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
  assert(!MF.empty() && "The function body is empty.");
  MF.getRegInfo().freezeReservedRegs(MF);

  LLVM_DEBUG(dbgs() << "RISCV64 -- getRaisedFunctionPrototype -- START\n");

  // Determine return type and argument types
  Type *ReturnType = determineReturnType();
  std::vector<Type *> ArgumentTypes = determineArgumentTypes();

  // Remove placeholder function
  Module *M = const_cast<Module *>(MF.getMMI().getModule());
  M->getFunctionList().remove(MF.getFunction());
  
  // Create function prototype
  FunctionType *FT = FunctionType::get(ReturnType, ArgumentTypes, false);
  Function *FunctionPrototype = Function::Create(FT, GlobalValue::ExternalLinkage, 
                                                 MF.getFunction().getName(), M);
  
  RaisedFunction = FunctionPrototype;

  LLVM_DEBUG(dbgs() << "RISCV64 -- getRaisedFunctionPrototype -- END\n");

  return RaisedFunction->getFunctionType();
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

MachineBasicBlock::const_reverse_iterator RISCV64MachineInstructionRaiser::findInstruction(unsigned Op, 
                                                                                           const MachineBasicBlock &MBB) {
  auto Pred = [Op](const MachineInstr &MI) { return MI.getDesc().getOpcode() == Op; };
  return std::find_if(MBB.rbegin(), MBB.rend(), Pred);
}
 
MachineBasicBlock::const_reverse_iterator RISCV64MachineInstructionRaiser::findInstruction(unsigned Reg, 
                                                                                           const MachineBasicBlock &MBB, 
                                                                                           MachineBasicBlock::const_reverse_iterator EndIt) {
  // Determine if register is being defined and that it is not tied to another register operand
  auto MOPred = [&Reg](const MachineOperand &MO) {
    return MO.isReg() && MO.getReg() == Reg && MO.isDef() && !MO.isTied();
  };

  // Find the iterator which defines the specified register  
  return std::find_if(MBB.rbegin(), EndIt, [&MOPred](const MachineInstr &MI) {
    return std::find_if(MI.operands_begin(), MI.operands_end(), MOPred) != MI.operands_end();
  });
}

std::vector<Type *> RISCV64MachineInstructionRaiser::determineArgumentTypes() {
  std::vector<Type *> ArgumentTypes;
  // TODO: discover argument types
  return ArgumentTypes;
}

Type *RISCV64MachineInstructionRaiser::determineReturnType() {
  for (const MachineBasicBlock &MBB : MF) {
    // Check if the basic block calls another function. If so, only search for the a0 register
    // after that instruction, because the a0 register might be defined as a function parameter.
    auto It = findInstruction(RISCV::JAL, MBB);
    // Check if basic block defines the a0 register as a return value, searching only after the last
    // call instruction or from the beginning of the basic block if no call instruction is present.
    if (findInstruction(RISCV::X10, MBB, It) != It) {
      // TODO: determine type of return register, for now default is used
      return Type::getIntNTy(MF.getFunction().getContext(), MF.getDataLayout().getPointerSizeInBits());
    }
  }
  return Type::getVoidTy(MF.getFunction().getContext());
}
