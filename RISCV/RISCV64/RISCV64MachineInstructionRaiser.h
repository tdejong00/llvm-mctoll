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
#include "RISCV64/RISCV64MachineInstructionUtils.h"
#include "RISCVELFUtils.h"
#include "Raiser/MachineInstructionRaiser.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Value.h"
#include <unordered_map>
#include <zconf.h>

namespace llvm {
namespace mctoll {

class RISCV64MachineInstructionRaiser : public MachineInstructionRaiser {
public:
  RISCV64MachineInstructionRaiser() = delete;
  RISCV64MachineInstructionRaiser(MachineFunction &MF, const ModuleRaiser *MR,
                                  MCInstRaiser *MCIR);

  /// Raises the machine function by traversing the machine basic blocks of the
  /// machine function in loop traversal order. Basic block will be created for
  /// each machine basic block with incrementing names; starting with "entry"
  /// and after that "bb.{X}", where X is the index of the machine basic block.
  /// The raised instructions will be added to the created basic blocks.
  bool raise() override;

  /// Creates a FunctionType and subsequently a Function by discovering the
  /// function signatures and stores the raised function (without instructions)
  /// in a member variable and returns the function type of that function.
  FunctionType *getRaisedFunctionPrototype() override;

  /// Gets the corresponding argument number of the register. When the register
  /// is not an argument register (x10-x17 i.e. a0-a7), -1 is returned.
  int getArgumentNumber(unsigned PReg) override;

  /// Gets the value currently stored in the register by first retrieving the
  /// value currently set in the register-value map. If this value is not set,
  /// and the given register is an argument register, the corresponding argument
  /// will be retrieved from the raised function.
  Value *getRegOrArgValue(unsigned PReg, int MBBNo) override;

  /// Gets the value corresponding to the machine operand. If the operand is a
  /// register, the value currently set in the register-value map is retrieved.
  /// If the operand is an immediate, a constant value is created corresponding
  /// to the immediate. Assumes that the operand is either a register or an
  /// immediate.
  Value *getRegOrImmValue(const MachineOperand &MOp);

  /// Not used, dummy implementation.
  bool buildFuncArgTypeVector(const std::set<MCPhysReg> &,
                              std::vector<Type *> &) override {
    return false;
  }

private:
  /// Raises the non-terminator machine instruction by calling the appropriate
  /// raise function and adds it (if applicable) to the basic block.
  bool raiseNonTerminatorInstruction(const MachineInstr &MI, BasicBlock *BB);

  /// Raises the terminator machine instruction by calling the appropriate
  /// raise function and adds it (if applicable) to the basic block.
  bool raiseTerminatorInstruction(const MachineInstr &MI, BasicBlock *BB);

  /// Raises the binary instruction by retrieving the values of the second
  /// operand (a register value) and the third operand (either a register value
  /// or a constant value) and creating a BinaryOperator instruction using
  /// these two values. The resulting instruction will be asigned to the
  /// register of the first operand in the register-value map.
  bool raiseBinaryOperation(BinaryOps BinOp, const MachineInstr &MI,
                            BasicBlock *BB);

  /// Raises a MV or LI instruction by retrieving the register or immediate
  /// value of the second operand and assigning it to the register of the first
  /// operand in the register-value map.
  bool raiseMoveInstruction(const MachineInstr &MI, BasicBlock *BB);

  /// Raises a load (LB, LH, LW, LD, ...) instruction by either retrieving the
  /// pointer stored at the stack offset (when the second operand is the stack
  /// pointer) or by retrieving the pointer stored in the register of the second
  /// operand (when the second operand is not the stack pointer and the offset
  /// is zero) and using that pointer to create a LoadInst. Other addressing
  /// modes are not yet supported. The resulting LoadInst will be assigned to
  /// the register of the first operand in the register-value map.
  bool raiseLoadInstruction(const MachineInstr &MI, BasicBlock *BB);

  /// Raises a store (SB, SH, SW, SD) instruction by retrieving the value of the
  /// first operand, creating an AllocaInst and storing the retrieved value to
  /// the created allocation by creating a StoreInst. The created allocation is
  /// assigned to the stack offset in the stack-value map.
  bool raiseStoreInstruction(const MachineInstr &MI, BasicBlock *BB);

  /// Raises an AUIPC instruction by looking at the ADDI instruction after the
  /// AUIPC. Using the offset of the instruction, text address and immediate, it
  /// is determined whether the global value is present in either the .rodata or
  /// .data sections of the ELF. If this is the case, a global variable is
  /// created and assigned to the register of the first operand in the
  /// register-value map.
  bool raiseGlobalInstruction(const MachineInstr &MI, BasicBlock *BB);

  /// Raises a JAL instruction by retrieving the called function, constructing
  /// the arguments vector based on the values stored in the argument registers,
  /// and creating a CallInst, which is stored in the return register (a0) in
  /// the register-value map.
  bool raiseCallInstruction(const MachineInstr &MI, BasicBlock *BB);

  /// Raises a JR instruction by creating a ReturnInst using the value
  /// stored in the return register in the register-value map.
  bool raiseReturnInstruction(const MachineInstr &MI, BasicBlock *BB);

  /// Raises a J instruction, by determining the basic block using the offset
  /// of the immediate operand and creating an unconditional branch instruction
  /// using the basic block as its target.
  bool raiseUnconditonalBranchInstruction(const MachineInstr &MI,
                                          BasicBlock *BB);

  /// Raises a branch instruction (BGE, BLT, BEQ, etc.), by creating a compare
  /// instruction using the LHS (register) and RHS (register or 0) of the
  /// machine instruction. The result is used to created a conditional branch
  /// instruction using the target basic block and successor basic block.
  bool raiseConditionalBranchInstruction(Predicate Pred, const MachineInstr &MI,
                                         BasicBlock *BB);

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
  RISCVELFUtils ELFUtils;

  /// A map from a MBB number to the corresponding BB.
  std::unordered_map<int, BasicBlock *> BasicBlocks;

  /// A map from a register number to the value stored in that register.
  std::unordered_map<unsigned, Value *> RegisterValues;

  /// A map from a stack offset to the value located at that address.
  std::unordered_map<signed, Value *> StackValues;
};

} // namespace mctoll
} // namespace llvm

#endif // LLVM_TOOLS_LLVM_MCTOLL_RISCV_RISCV64_RISCV64MACHINEINSTRUCTIONRAISER_H
