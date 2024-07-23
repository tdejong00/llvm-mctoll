//===-- RISCV64MachineInstructionRaiser.h -----------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains the declaration of RISCV64MachineInstructionRaiser
// class for use by llvm-mctoll.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_TOOLS_LLVM_MCTOLL_RISCV_RISCV64_RISCV64MACHINEINSTRUCTIONRAISER_H
#define LLVM_TOOLS_LLVM_MCTOLL_RISCV_RISCV64_RISCV64MACHINEINSTRUCTIONRAISER_H

#include "MCInstRaiser.h"
#include "RISCV64/RISCV64FunctionPrototypeDiscoverer.h"
#include "RISCV64/RISCV64MachineInstructionRaiserUtils.h"
#include "RISCV64/RISCV64ValueTracker.h"
#include "RISCVELFUtils.h"
#include "Raiser/MachineInstructionRaiser.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Value.h"
#include <unordered_map>
#include <unordered_set>
#include <zconf.h>

namespace llvm {
namespace mctoll {

class RISCV64MachineInstructionRaiser : public MachineInstructionRaiser {
public:
  RISCV64MachineInstructionRaiser() = delete;
  RISCV64MachineInstructionRaiser(MachineFunction &MF, const ModuleRaiser *MR,
                                  MCInstRaiser *MCIR);

  /// Prints the values of the argument registers.
  void printArgumentRegisters(int MBBNo);

  /// Raises the machine function by traversing the machine basic blocks of the
  /// machine function in loop traversal order. A basic block will be created
  /// for each machine basic block. The raised instructions of the machine
  /// basic blocks will be added to the created basic blocks.
  bool raise() override;

  /// Creates a FunctionType and a Function by discovering the function
  /// signatures. The raised function (without instructions) will be stored
  /// in a member variable, and returns the function type of that function.
  FunctionType *getRaisedFunctionPrototype() override;

  /// Gets the corresponding argument number of the register. When the register
  /// is not an argument register (x10-x17 i.e. a0-a7), -1 is returned.
  int getArgumentNumber(unsigned int PReg) override;

  /// Gets the value for the specified register number by first retrieving the
  /// value currently assigned to the register. If this value is not set,
  /// and the given register is an argument register, the corresponding argument
  /// will be retrieved from the raised function.
  Value *getRegOrArgValue(unsigned int PReg, int MBBNo) override;

  /// Gets the value corresponding to the machine operand. If the operand is a
  /// register, the value currently assigned to it is retrieved. Otherwise, a
  /// constant value is created corresponding to the immediate.
  Value *getRegOrImmValue(const MachineOperand &MOp, int MBBNo);

  /// Gets the basic block created for the specified machine basic block.
  /// Returns nullptr if the block is not (yet) created.
  BasicBlock *getBasicBlock(int MBBNo);

  /// Not used, dummy implementation.
  bool buildFuncArgTypeVector(const std::set<MCPhysReg> &,
                              std::vector<Type *> &) override {
    return false;
  }

private:
  /// Coerces the type of the specified destination type upon the specified
  /// value, by adding cast or convert instructions using the specified builder.
  /// When the type of the value is already the same as the destination
  /// type, nothing happens. Returns false when the types are not equal and
  /// no coercion is possible.
  bool coerceType(Value *&Val, Type *DestTy, IRBuilder<> &Builder);

  /// Widens the type of either the LHS or the RHS, depending on which
  /// is wider, by adding sign-extend instructions using the specified
  /// builder. When the types of the two values are equal, nothing
  /// happens. Returns false when the types are not equal and no
  /// widening is possible.
  bool widenType(Value *&LHS, Value *&RHS, IRBuilder<> &Builder);

  /// Raises the non-terminator machine instruction by calling the appropriate
  /// raise function and adds it (if applicable) to the basic block.
  bool raiseNonTerminator(const MachineInstr &MI, int MBBNo);

  /// Raises the terminator machine instruction by calling the appropriate
  /// raise function and adds it (if applicable) to the basic block.
  bool raiseTerminator(ControlTransferInfo *Info);

  /// Raises the binary operation by creating a BinaryOperator and assigning it
  /// to the register of the first operand. Integer types are widened in case
  /// of a mismatch.
  bool raiseBinaryOperation(BinaryOps BinOp, const MachineInstr &MI, int MBBNo);

  /// Raises a MV or LI instruction by assigning the value to the register of
  /// the first operand.
  bool raiseMove(const MachineInstr &MI, int MBBNo);

  /// Raises a LB, LH, LW, or LD instruction by creating a LoadInst instruction
  /// using the value of the register of the second operand as the pointer and
  /// the immediate of the third operand as the offset. The create instruction
  /// will be assigned to the register of the first operand.
  bool raiseLoad(const MachineInstr &MI, int MBBNo);

  /// Raises a SB, SH, SW, or SD instruction by creating a StoreInst instruction
  /// using the value of the register of the first operand as the value to
  /// store, the value of the register of the second operand as the pointer to
  /// store to, and the immediate of the third operand as the offset.
  bool raiseStore(const MachineInstr &MI, int MBBNo);

  /// Raises an AUIPC or LUI instruction by resolving the PC-relative or
  /// absoluteaddress to a global variable, creating it when it has not yet
  /// been created. The global variable is assigned to the first register of
  /// the accompanying instruction.
  bool raisePCRelativeOrAbsoluteAccess(const MachineInstr &MI, int MBBNo);

  /// Raises a JAL instruction by retrieving the called function, constructing
  /// the arguments vector based on the values stored in the argument registers,
  /// and creating a CallInst, which is assigned to the return register.
  bool raiseCall(const MachineInstr &MI, int MBBNo);

  /// Raises a JR instruction by creating a ReturnInst using the value assigned
  /// to the return register.
  bool raiseReturn(const MachineInstr &MI, int MBBNo);

  /// Raises a J instruction by determining the basic block using the offset
  /// of the immediate operand and creating an unconditional branch instruction
  /// using the basic block as its target.
  bool raiseUnconditonalBranch(ControlTransferInfo *Info);

  /// Raises a branch instruction (BGE, BLT, BEQ, etc.), by creating a compare
  /// instruction using the LHS (register) and RHS (register or 0) of the
  /// machine instruction. The result is used to created a conditional branch
  /// instruction using the target basic block and successor basic block.
  bool raiseConditionalBranch(Predicate Pred, ControlTransferInfo *Info);

  /// Gets the function being called by the instruction by first checking if the
  /// function is known in the module raiser. If not, the .plt section is used
  /// for finding the called function.
  Function *getCalledFunction(const MachineInstr &MI) const;

  /// Gets the basic block using the number of the machine basic block, which
  /// the machine instruction is a part of. Returns nullptr if MBB could not
  /// be found, or when the basic block has not been created.
  BasicBlock *getBasicBlockAtOffset(const MachineInstr &MI, uint64_t Offset);

  LLVMContext &C;
  MCInstRaiser *MCIR;

  RISCV64FunctionPrototypeDiscoverer FunctionPrototypeDiscoverer;
  RISCV64ValueTracker ValueTracker;
  RISCVELFUtils ELFUtils;

  /// A ConstantInt representing the number zero using the default int type.
  ConstantInt *Zero;

  /// A map from a MBB number to the corresponding BB.
  std::unordered_map<int, BasicBlock *> BasicBlocks;

  std::unordered_set<const MachineInstr *> SkippedInstructions;
};

} // namespace mctoll
} // namespace llvm

#endif // LLVM_TOOLS_LLVM_MCTOLL_RISCV_RISCV64_RISCV64MACHINEINSTRUCTIONRAISER_H
