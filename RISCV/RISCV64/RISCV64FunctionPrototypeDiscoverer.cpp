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

    if (Begin->getOpcode() == C_LI) {
      return getDefaultIntType(C);
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
        if (Prev != nullptr && Prev->getOpcode() == AUIPC &&
            isAddI(It->getOpcode()) && Next != nullptr &&
            !(isLoad(It->getOpcode()) &&
              getAlign(It->getOpcode()) == DoubleWordAlign)) {
          return Type::getInt64Ty(C);
        }

        // Defining instruction does not load a pointer
        break;
      }
    }

    // Assume integer return type by default
    return getDefaultIntType(C);
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
      // Argument register is moved to a local register and not yet defined
      if (It->getOpcode() == C_MV || toBinaryOperation(It->getOpcode()) != BinaryOps::BinaryOpsEnd) {
        const MachineOperand &MOp2 = It->getOperand(1);
        assert(MOp2.isReg());
        if (isArgReg(MOp2.getReg()) &&
            !isRegisterDefined(MOp2.getReg(), MBB.instr_begin(), It)) {
          ArgumentTypes.push_back(getDefaultIntType(C));
        }
      }
      // Argument register is stored to a stack slot and not yet defined
      else if (It->getOpcode() == SD) {
        const MachineOperand &MOp1 = It->getOperand(0);
        assert(MOp1.isReg());
        if (isArgReg(MOp1.getReg()) &&
            !isRegisterDefined(MOp1.getReg(), MBB.instr_begin(), It)) {
          ArgumentTypes.push_back(Type::getInt64Ty(C));
        }
      }
    }
  }

  return ArgumentTypes;
}
