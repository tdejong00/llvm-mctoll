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
#include "RISCV64MachineInstructionUtils.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/IR/CallingConv.h"
#include <cassert>

using namespace llvm;
using namespace llvm::mctoll;
using namespace llvm::mctoll::RISCV64MachineInstructionUtils;

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
    auto REnd = findInstructionByOpcode(MBB, RISCV::JAL, MBB.instr_rend());

    // Check if basic block defines the a0 register as a return value, searching
    // only after the last call instruction (or from the beginning of the basic
    // block if no call instruction is present).
    auto RBegin = findInstructionByRegNo(MBB, RISCV::X10, REnd);

    // Return value not defined
    if (RBegin == REnd) {
      continue;
    }

    const MachineOperand &MOp2 = RBegin->getOperand(1);
    assert(RBegin->getOpcode() == RISCV::C_MV && MOp2.isReg());

    // Determine if pointer type based on the instruction which defines
    // the register whose contents are moved to the return register.
    for (auto It = RBegin; It != REnd; ++It) {
      const MachineInstr *Prev = It->getPrevNode();
      const MachineInstr *Next = It->getNextNode();
      if (!Prev || !Next) {
        continue;
      }

      if (It->definesRegister(MOp2.getReg())) {
        // Defining instruction loads a pointer
        if (It->getOpcode() == RISCV::LD ||
            It->getOpcode() == RISCV::C_LD) {
          return getDefaultPtrType(C);
        }

        // Defining instruction loads a global variable pointer
        if (Prev->getOpcode() == RISCV::AUIPC &&
            isAddI(It->getOpcode()) &&
            getInstructionType(Next->getOpcode()) != InstructionType::LOAD) {
          return getDefaultPtrType(C);
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
      // When an argument register, whose value is not yet defined, is moved
      // to a local register, it must be an integral value.
      if (It->getOpcode() == RISCV::C_MV) {
        const MachineOperand &MOp2 = It->getOperand(1);
        assert(MOp2.isReg());
        if (isArgReg(MOp2.getReg()) &&
            !isRegisterDefined(MOp2.getReg(), MBB.instr_begin(), It)) {
          ArgumentTypes.push_back(getDefaultIntType(C));
        }
      }
      // When an argument register, whose value is not yet defined, is stored
      // using a 64-bit store, it must be a pointer value.
      else if (It->getOpcode() == RISCV::SD) {
        const MachineOperand &MOp1 = It->getOperand(0);
        assert(MOp1.isReg());
        if (isArgReg(MOp1.getReg()) &&
            !isRegisterDefined(MOp1.getReg(), MBB.instr_begin(), It)) {
          ArgumentTypes.push_back(getDefaultPtrType(C));
        }
      }
    }
  }

  return ArgumentTypes;
}
