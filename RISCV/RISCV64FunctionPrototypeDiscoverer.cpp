//===-- RISCV64FunctionPrototypeDiscoverer.cpp ------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains the definition of the RISCV64FunctionPrototypeDiscoverer
// class for use by llvm-mctoll.
//
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/RISCVMCTargetDesc.h"
#include "RISCV64FunctionPrototypeDiscoverer.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineModuleInfo.h"

using namespace llvm;
using namespace llvm::mctoll;

/// Finds the operand which has the specified register number inside the specified instruction and
/// validates whether the register is defined or used based on the value of `IsDefinition`.
inline MachineInstr::const_mop_iterator findOperand(unsigned RegNo, bool IsDefinition, const MachineInstr &MI) {
  auto Pred = [&RegNo, &IsDefinition](const MachineOperand &MO) {
    return MO.isReg() && MO.getReg() == RegNo 
      && ((IsDefinition && MO.isDef() && !MO.isTied()) || (!IsDefinition && !MO.isDef()));
  };
  return std::find_if(MI.operands_begin(), MI.operands_end(), Pred);
}

/// Finds the instruction which has the specified opcode inside the specified basic block.
inline MachineBasicBlock::const_reverse_iterator findInstructionForOp(unsigned Op, const MachineBasicBlock &MBB) {
  auto Pred = [&Op](const MachineInstr &MI) { return MI.getDesc().getOpcode() == Op; };
  return std::find_if(MBB.rbegin(), MBB.rend(), Pred);
}
 
/// Finds the instruction which defines the register which has the specified register number inside
/// the specified basic block. Will only search up to the `CallIt` instruction iterator.
inline MachineBasicBlock::const_reverse_iterator findInstructionForRegNo(unsigned RegNo, const MachineBasicBlock &MBB, 
                                                                  MachineBasicBlock::const_reverse_iterator CallIt) {
  auto Pred = [&RegNo](const MachineInstr &MI) {
    return findOperand(RegNo, true, MI) != MI.operands_end();
  };
  return std::find_if(MBB.rbegin(), CallIt, Pred);
}

/// Finds the instruction which represents the end of the stack prologue.
inline MachineBasicBlock::const_iterator findInstructionPrologueEnd(const MachineBasicBlock &MBB) {
  auto Pred = [](const MachineInstr &MI) {
    return findOperand(RISCV::X8, true, MI) != MI.operands_end()
      && findOperand(RISCV::X2, false, MI) != MI.operands_end();
  };
  return std::find_if(MBB.begin(), MBB.end(), Pred);
}

/// Determines whether the return value of the function is a pointer.
/// Iterates through the instructions from `BeginIt` to `EndIt` until an ADDI instruction
/// is found. If this instruction is preceded by an AUIPC instruction and NOT subceeded by
/// and LD instruction, it is most likely that the function returns a pointer type.
inline bool isPointerType(MachineBasicBlock::const_reverse_iterator BeginIt,
                          MachineBasicBlock::const_reverse_iterator EndIt) {
  auto Pred = [](const MachineInstr &MI) {
    return MI.getOpcode() == RISCV::ADDI 
      && MI.getPrevNode() != nullptr && MI.getPrevNode()->getOpcode() == RISCV::AUIPC
      && MI.getNextNode() != nullptr && MI.getNextNode()->getOpcode() != RISCV::C_LD;
  };
  return std::find_if(BeginIt, EndIt, Pred) != EndIt;
}

Function *RISCV64FunctionPrototypeDiscoverer::discoverFunctionPrototype() const {
  assert(!MF.empty() && "The function body is empty.");

  // Determine return type and argument types
  Type *ReturnType = discoverReturnType();
  std::vector<Type *> ArgumentTypes = discoverArgumentTypes();

  // Remove placeholder function
  Module *M = const_cast<Module *>(MF.getMMI().getModule());
  M->getFunctionList().remove(MF.getFunction());
  
  // Create function prototype
  FunctionType *FT = FunctionType::get(ReturnType, ArgumentTypes, false);
  return Function::Create(FT, GlobalValue::ExternalLinkage, MF.getFunction().getName(), M);
}

Type *RISCV64FunctionPrototypeDiscoverer::discoverReturnType() const {
  for (const MachineBasicBlock &MBB : MF) {
    // Check if the basic block calls another function. If so, only search for the a0 register
    // after that instruction, because the a0 register might be defined as a function parameter.
    auto CallIt = findInstructionForOp(RISCV::JAL, MBB);
    // Check if basic block defines the a0 register as a return value, searching only after the last
    // call instruction or from the beginning of the basic block if no call instruction is present.
    auto DefineIt = findInstructionForRegNo(RISCV::X10, MBB, CallIt);
    if (DefineIt != CallIt) {
      // TODO: discover if return value is a pointer
      // TODO: discover size of return value
      return Type::getInt64Ty(MF.getFunction().getContext());
    }
  }
  return Type::getVoidTy(MF.getFunction().getContext());
}

std::vector<Type *> RISCV64FunctionPrototypeDiscoverer::discoverArgumentTypes() const {
  std::vector<Type *> ArgumentTypes;

  for (const MachineBasicBlock &MBB : MF) {
    auto It = findInstructionPrologueEnd(MBB);
    ++It;
    // Check if instruction moves parameter to local register or stores parameter on stack
    while (It->getOpcode() == RISCV::C_MV || It->getOpcode() == RISCV::SD) {
      // Loop over parameter registers (a0 - a7)
      for (unsigned RegNo = RISCV::X10; RegNo < RISCV::X17; RegNo++) {
        const auto *MO = findOperand(RegNo, false, *It); 
        
        // Parameter register is moved to local register or is stored on the stack
        if (MO != It->operands_end()) {
          // TODO: discover if return value is a pointer
          // TODO: discover size of return value
          ArgumentTypes.push_back(Type::getInt64Ty(MF.getFunction().getContext()));
        }
      }
      ++It;
    }
  }

  return ArgumentTypes;
}
