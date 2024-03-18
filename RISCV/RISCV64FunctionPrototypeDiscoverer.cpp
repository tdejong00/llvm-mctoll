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
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/CodeGen/MachineOperand.h"

using namespace llvm;
using namespace llvm::mctoll;
using namespace llvm::mctoll::riscv_utils;

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
  return Function::Create(FT, GlobalValue::ExternalLinkage,
                          MF.getFunction().getName(), M);
}

Type *RISCV64FunctionPrototypeDiscoverer::discoverReturnType() const {
  Type *Ty = Type::getVoidTy(C);

  for (const MachineBasicBlock &MBB : MF) {
    // Check if the basic block calls another function. If so, only search for
    // the a0 register after that instruction, because the a0 register might be
    // defined as a function parameter.
    auto CallIt = findInstructionByOpcode(MBB, RISCV::JAL, MBB.instr_rend());

    // Check if basic block defines the a0 register as a return value, searching
    // only after the last call instruction or from the beginning of the basic
    // block if no call instruction is present.
    auto DefineIt = findInstructionByRegNo(MBB, RISCV::X10, CallIt);

    if (DefineIt != CallIt) {
      // TODO: discover if return value is a pointer
      // TODO: discover size of return value
      Ty = getDefaultIntType(MF);
      Type::getIntNTy(C, MF.getDataLayout().getPointerSizeInBits());
      break;
    }
  }

  return Ty;
}

std::vector<Type *>
RISCV64FunctionPrototypeDiscoverer::discoverArgumentTypes() const {
  std::vector<Type *> ArgumentTypes;

  for (MachineBasicBlock &MBB : MF) {
    // Check if the instruction moves the argument to a local register or if it
    // stores the argument on the stack. Only check the first move/store
    // instructions after the prolog.
    auto It = skipProlog(MBB);
    while (It->getOpcode() == RISCV::C_MV || It->getOpcode() == RISCV::SD) {
      // Loop over parameter registers (a0 - a7)
      for (unsigned RegNo = RISCV::X10; RegNo < RISCV::X17; RegNo++) {
        // Get source operand
        const MachineOperand &SrcOp = It->getOperand(1);

        // Parameter register is moved to local register or is stored on the
        // stack
        if (SrcOp.isReg() && SrcOp.getReg() == RegNo) {
          // TODO: discover if return value is a pointer
          // TODO: discover size of return value
          ArgumentTypes.push_back(getDefaultIntType(MF));
        }
      }
      ++It;
    }
  }

  return ArgumentTypes;
}
