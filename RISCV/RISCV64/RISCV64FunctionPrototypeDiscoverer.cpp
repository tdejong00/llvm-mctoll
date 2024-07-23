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

#include "RISCV64FunctionPrototypeDiscoverer.h"
#include "MCTargetDesc/RISCVMCTargetDesc.h"
#include "RISCV64MachineInstructionRaiserUtils.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/IR/CallingConv.h"
#include "llvm/IR/Instruction.h"
#include <cassert>
#include <cstdint>

using namespace llvm;
using namespace mctoll;
using namespace RISCV;
using namespace RISCV64MachineInstructionRaiserUtils;

Function *
RISCV64FunctionPrototypeDiscoverer::discoverFunctionPrototype() const {
  assert(!MF.empty() && "The function body is empty.");

  // Determine return type and argument types
  Type *ReturnType = discoverReturnType();
  std::vector<Type *> ArgumentTypes = discoverArgumentTypes();

  // Remove placeholder function
  Module *M = const_cast<Module *>(MF.getMMI().getModule());
  M->getFunctionList().remove(MF.getFunction());

  // Create function prototype
  FunctionType *FT = FunctionType::get(ReturnType, ArgumentTypes, false);
  Function *F = Function::Create(FT, GlobalValue::ExternalLinkage,
                                 MF.getFunction().getName(), M);
  F->setCallingConv(CallingConv::C);
  F->setDSOLocal(true);
  return F;
}

Type *RISCV64FunctionPrototypeDiscoverer::discoverReturnType() const {
  Type *Ty = Type::getVoidTy(C);

  for (const MachineBasicBlock &MBB : MF) {
    // Check if the basic block calls another function. If so, only search for
    // the a0 register after that instruction, because the a0 register might be
    // defined as a function parameter.
    auto End =
        findInstructionByOpcode(JAL, MBB.instr_rbegin(), MBB.instr_rend());

    // Check if basic block defines the a0 register as a return value, searching
    // only after the last call instruction (or from the beginning of the basic
    // block if no call instruction is present).
    auto Begin = findInstructionByRegNo(X10, MBB.instr_rbegin(), End);

    // Return value not defined
    if (Begin == End) {
      continue;
    }

    // Move immediate always means 32-bit return type
    if (Begin->getOpcode() == C_LI) {
      return Type::getInt32Ty(C);
    }

    const MachineOperand &MOp2 = Begin->getOperand(1);
    assert(MOp2.isReg());

    // Determine if pointer type based on the instruction which defines
    // the register whose contents are moved to the return register.
    for (auto It = Begin; It != End; ++It) {
      if (It->definesRegister(MOp2.getReg())) {
        // Defining instruction loads a pointer
        if (isLoad(It->getOpcode()) &&
            getAlign(It->getOpcode()) == DoubleWordAlign) {
          return Type::getInt64Ty(C);
        }

        // Defining instruction loads a global variable pointer
        const MachineInstr *Prev = It->getPrevNode();
        const MachineInstr *Next = It->getNextNode();
        if (Prev != nullptr &&
            (Prev->getOpcode() == AUIPC || Prev->getOpcode() == LUI ||
             Prev->getOpcode() == C_LUI) &&
            isAddI(It->getOpcode()) && Next != nullptr &&
            !(isLoad(It->getOpcode()) &&
              getAlign(It->getOpcode()) == DoubleWordAlign)) {
          return Type::getInt64Ty(C);
        }

        // Defining instruction does not load a pointer
        break;
      }
    }

    // Assume 32-bit integer return type by default
    return Type::getInt32Ty(C);
  }

  return Ty;
}

std::vector<Type *>
RISCV64FunctionPrototypeDiscoverer::discoverArgumentTypes() const {
  std::vector<Type *> ArgumentTypes;

  for (MachineBasicBlock &MBB : MF) {
    // Only consider entry blocks
    if (!MBB.isEntryBlock()) {
      continue;
    }

    // Loop over instructions and check for use of argument
    // registers whose values are not yet defined.
    for (auto It = MBB.instr_begin(); It != MBB.instr_end(); ++It) {
      unsigned int Op = It->getOpcode();

      // Only consider moves, stores, and binary operations
      if (Op != C_MV && !isStore(Op) &&
          toBinaryOperation(Op) == Instruction::BinaryOpsEnd) {
        continue;
      }

      const MachineOperand &MOp =
          isStore(It->getOpcode()) ? It->getOperand(0) : It->getOperand(1);
      assert(MOp.isReg());

      // Register is argument register and is not defined -> argument
      if (isArgReg(MOp.getReg()) &&
          !isRegisterDefined(MOp.getReg(), MBB.instr_begin(), It)) {
        // Determine type based on
        uint64_t Align = getAlign(It->getOpcode());
        Type *Ty = Align == DoubleWordAlign ? Type::getInt64Ty(C)
                                            : Type::getInt32Ty(C);
        ArgumentTypes.push_back(Ty);
      }
    }
  }

  return ArgumentTypes;
}
