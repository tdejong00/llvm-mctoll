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

#include "RISCV64MachineInstructionRaiser.h"
#include "MCInstRaiser.h"
#include "MCTargetDesc/RISCVAsmBackend.h"
#include "MCTargetDesc/RISCVMCTargetDesc.h"
#include "ModuleRaiser.h"
#include "RISCV64FunctionPrototypeDiscoverer.h"
#include "RISCV64MachineInstructionUtils.h"
#include "RISCVELFUtils.h"
#include "RISCVModuleRaiser.h"
#include "Raiser/MachineFunctionRaiser.h"
#include "llvm/ADT/APInt.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/CodeGen/GlobalISel/MachineIRBuilder.h"
#include "llvm/CodeGen/LivePhysRegs.h"
#include "llvm/CodeGen/LoopTraversal.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineJumpTableInfo.h"
#include "llvm/CodeGen/MachineMemOperand.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/CodeGen/PseudoSourceValue.h"
#include "llvm/CodeGen/Register.h"
#include "llvm/CodeGen/RegisterPressure.h"
#include "llvm/CodeGen/SelectionDAG.h"
#include "llvm/CodeGen/TargetRegisterInfo.h"
#include "llvm/CodeGen/TargetSubtargetInfo.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Value.h"
#include "llvm/MC/MCInst.h"
#include "llvm/Object/SymbolicFile.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"
#include <cassert>
#include <cstdint>
#include <netinet/in.h>
#include <string>
#include <sys/types.h>
#include <vector>
#include <zconf.h>

#define DEBUG_TYPE "mctoll"

using namespace llvm;
using namespace llvm::mctoll;
using namespace llvm::mctoll::RISCV64MachineInstructionUtils;

#define ENABLE_RAISING_DEBUG_INFO

/// Prints a result message for the machine instruction with an optional message
void printResult(const MachineInstr &MI, std::string Result,
                 std::string Reason = "") {
#ifdef ENABLE_RAISING_DEBUG_INFO
  LLVM_DEBUG(
      dbgs() << Result << ": "; MI.print(dbgs(), true, false, false, false);
      dbgs() << "\033[0;39m";
      if (!Reason.empty()) { dbgs() << ": " << Reason; } dbgs() << "\n";);
#endif
}

void printSuccess(const MachineInstr &MI, std::string Reason = "") {
  printResult(MI, "\033[32mSuccess", Reason);
}
void printFailure(const MachineInstr &MI, std::string Reason = "") {
  printResult(MI, "\033[31mFailure", Reason);
}
void printSkipped(const MachineInstr &MI, std::string Reason = "") {
  printResult(MI, "\033[90mSkipped", Reason);
}

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
    : MachineInstructionRaiser(MF, MR, MCIR), C(MF.getFunction().getContext()),
      MCIR(MCIR), FunctionPrototypeDiscoverer(MF), ELFUtils(MR, C) {
  // Initialize hardwired zero register
  RegisterValues[RISCV::X0] = ConstantInt::get(getDefaultIntType(C), 0);
}

bool RISCV64MachineInstructionRaiser::raise() {
  LoopTraversal Traversal;
  LoopTraversal::TraversalOrder TraversalOrder = Traversal.traverse(MF);

  // Traverse basic blocks of machine function in LoopTraversal
  // order and raise all non-terminator instructions
  for (LoopTraversal::TraversedMBBInfo MBBInfo : TraversalOrder) {
    MachineBasicBlock *MBB = MBBInfo.MBB;

    // Determine name of basic block
    int MBBNo = MBB->getNumber();
    std::string BBName = "entry";
    if (MBBNo > 0) {
      BBName = "bb." + std::to_string(MBBNo);
    }

    // Create basic block
    BasicBlock *BB = BasicBlock::Create(C, BBName, RaisedFunction);

    // Store basic block for reference during direct branch instructions
    BasicBlocks[MBBNo] = BB;

    // Loop over machine instructions of basic block and raise each instruction.
    for (const MachineInstr &MI : MBB->instrs()) {
      bool WasAUIPC = MI.getPrevNode() != nullptr &&
                      MI.getPrevNode()->getOpcode() == RISCV::AUIPC;

      // Skip raising of prolog and epilog instructions,
      // as these do not contain any needed information
      if (isPrologInstruction(MI)) {
        printSkipped(MI, "Skipped raising of prolog instruction");
        continue;
      }
      if (isEpilogInstruction(MI)) {
        printSkipped(MI, "Skipped raising of epilog instruction");
        continue;
      }

      // The instruction after an AUIPC is already handled
      //  during raising of the AUIPC instrucion.
      if (WasAUIPC) {
        printSkipped(MI, "Already raised by previous instruction");
        continue;
      }

      // Skip raising terminator instructions for now
      if (MI.isTerminator() &&
          getInstructionType(MI.getOpcode()) != InstructionType::RETURN) {
        printSkipped(MI, "Skipped raising terminator instruction");
        continue;
      }

      // Raise non-terminator instruction
      if (raiseNonTerminatorInstruction(MI, BB)) {
        printSuccess(MI);
      }
    }
  }

  LLVM_DEBUG(dbgs() << "\nBefore raising terminator instructions:\n");
  LLVM_DEBUG(RaisedFunction->dump());

  // Second pass, raise all terminator instructions
  for (MachineBasicBlock &MBB : MF) {
    // Check if basic block hase terminator instructions. If not,
    // add fall through branch instruction to successor basic block.
    if (MBB.getFirstTerminator() == MBB.end() && !MBB.succ_empty()) {
      BasicBlock *CurrBB = BasicBlocks[MBB.getNumber()];
      assert(CurrBB != nullptr && "BB has not been created");

      BasicBlock *SuccBB = BasicBlocks[MBB.getNumber() + 1];
      assert(SuccBB != nullptr && "Successor BB has not been created");

      IRBuilder<> Builder(CurrBB);

      Builder.CreateBr(SuccBB);

      continue;
    }

    for (MachineInstr &MI : MBB) {
      if (!MI.isUnconditionalBranch() && !MI.isConditionalBranch()) {
        continue;
      }

      BasicBlock *BB = BasicBlocks[MBB.getNumber()];
      if (raiseTerminatorInstruction(MI, BB)) {
        printSuccess(MI);
      }
    }
  }

  LLVM_DEBUG(dbgs() << "\nAfter raising terminator instructions:\n");
  LLVM_DEBUG(RaisedFunction->dump());

  return true;
}

FunctionType *RISCV64MachineInstructionRaiser::getRaisedFunctionPrototype() {
  RaisedFunction = FunctionPrototypeDiscoverer.discoverFunctionPrototype();
  return RaisedFunction->getFunctionType();
}

int RISCV64MachineInstructionRaiser::getArgumentNumber(unsigned PReg) {
  unsigned ArgReg = PReg - RISCV::X10;
  if (ArgReg < 8) {
    return ArgReg;
  }
  return -1;
}

Value *RISCV64MachineInstructionRaiser::getRegOrArgValue(unsigned PReg,
                                                         int MBBNo = 0) {
  Value *Val = RegisterValues[PReg];

  int ArgNo = getArgumentNumber(PReg);
  int NumArgs = RaisedFunction->getFunctionType()->getNumParams();

  // Attempt to get value from function arguments
  if (Val == nullptr && ArgNo >= 0 && ArgNo < NumArgs) {
    Val = RaisedFunction->getArg(ArgNo);
  }

  return Val;
}

Value *
RISCV64MachineInstructionRaiser::getRegOrImmValue(const MachineOperand &MOp) {
  assert(MOp.isReg() || MOp.isImm());

  if (MOp.isReg()) {
    return getRegOrArgValue(MOp.getReg());
  }

  return ConstantInt::get(getDefaultIntType(C), MOp.getImm());
}

bool RISCV64MachineInstructionRaiser::raiseNonTerminatorInstruction(
    const MachineInstr &MI, BasicBlock *BB) {
  InstructionType Type = getInstructionType(MI.getOpcode());

  assert(Type != InstructionType::UNCONDITIONAL_BRANCH &&
         Type != InstructionType::CONDITIONAL_BRANCH);

  if (Type == InstructionType::BINOP) {
    BinaryOps BinOp = toBinaryOperation(MI.getOpcode());

    if (BinOp == BinaryOps::BinaryOpsEnd) {
      printFailure(MI, "Unimplemented or unknown binary operation");
      return false;
    }

    return raiseBinaryOperation(BinOp, MI, BB);
  }

  switch (Type) {
  case InstructionType::NOP:
    return true;
  case InstructionType::MOVE:
    return raiseMoveInstruction(MI, BB);
  case InstructionType::LOAD:
    return raiseLoadInstruction(MI, BB);
  case InstructionType::STORE:
    return raiseStoreInstruction(MI, BB);
  case InstructionType::GLOBAL:
    return raiseGlobalInstruction(MI, BB);
  case InstructionType::CALL:
    return raiseCallInstruction(MI, BB);
  case InstructionType::RETURN:
    return raiseReturnInstruction(MI, BB);
  default:
    printFailure(MI, "Unimplemented or unknown instruction");
    return false;
  }
}

bool RISCV64MachineInstructionRaiser::raiseTerminatorInstruction(
    const MachineInstr &MI, BasicBlock *BB) {
  InstructionType Type = getInstructionType(MI.getOpcode());

  assert(Type == InstructionType::UNCONDITIONAL_BRANCH ||
         Type == InstructionType::CONDITIONAL_BRANCH);

  switch (Type) {
  case InstructionType::UNCONDITIONAL_BRANCH:
    return raiseUnconditonalBranchInstruction(MI, BB);
  case InstructionType::CONDITIONAL_BRANCH: {
    Predicate Pred = toPredicate(MI.getOpcode());
    return raiseConditionalBranchInstruction(Pred, MI, BB);
  }
  default:
    printFailure(MI, "Unimplemented or unknown instruction");
    return false;
  }
}

bool RISCV64MachineInstructionRaiser::raiseBinaryOperation(
    BinaryOps BinOp, const MachineInstr &MI, BasicBlock *BB) {
  IRBuilder<> Builder(BB);

  const MachineOperand &MOp1 = MI.getOperand(0);
  const MachineOperand &MOp2 = MI.getOperand(1);
  const MachineOperand &MOp3 = MI.getOperand(2);

  assert(MOp1.isReg() && MOp2.isReg() && (MOp3.isReg() || MOp3.isImm()));

  // Instructions like `addi s0,$` should load value at stack offset
  if (BinOp == BinaryOps::Add && MOp2.getReg() == RISCV::X8 && MOp3.isImm()) {
    RegisterValues[MOp1.getReg()] = StackValues[MOp3.getImm()];
    return true;
  }

  Value *LHS = getRegOrArgValue(MOp2.getReg());
  if (LHS == nullptr) {
    printFailure(MI, "LHS value of add instruction not set");
    return false;
  }

  Value *RHS = getRegOrImmValue(MOp3);
  if (RHS == nullptr) {
    printFailure(MI, "RHS value of add instruction not set");
    return false;
  }

  RegisterValues[MOp1.getReg()] = Builder.CreateBinOp(BinOp, LHS, RHS);

  return true;
}

bool RISCV64MachineInstructionRaiser::raiseMoveInstruction(
    const MachineInstr &MI, BasicBlock *BB) {
  IRBuilder<> Builder(BB);

  const MachineOperand &MOp1 = MI.getOperand(0);
  const MachineOperand &MOp2 = MI.getOperand(1);

  assert(MOp1.isReg() && (MOp2.isReg() || MOp2.isImm()));

  Value *Val = getRegOrImmValue(MOp2);
  if (Val == nullptr) {
    printFailure(MI, "Register value of move instruction not set");
    return false;
  }

  RegisterValues[MOp1.getReg()] = Val;

  return true;
}

bool RISCV64MachineInstructionRaiser::raiseLoadInstruction(
    const MachineInstr &MI, BasicBlock *BB) {
  IRBuilder<> Builder(BB);

  const MachineOperand &MOp1 = MI.getOperand(0);
  const MachineOperand &MOp2 = MI.getOperand(1);
  const MachineOperand &MOp3 = MI.getOperand(2);

  assert(MOp1.isReg() && MOp2.isReg() && MOp3.isImm());

  Value *Ptr = nullptr;

  // Load from stack
  if (MOp2.getReg() == RISCV::X8) {
    Ptr = StackValues[MOp3.getImm()];
    if (Ptr == nullptr) {
      printFailure(MI, "Stack value of load instruction not set");
      return false;
    }
  }
  // Load value from address specified in register
  else if (MOp3.getImm() == 0) {
    Ptr = getRegOrArgValue(MOp2.getReg());
  }

  if (Ptr == nullptr) {
    printFailure(MI, "Pointer of load instruction not set");
    return false;
  }

  Type *Ty = getDefaultIntType(C);
  if (MI.getOpcode() == RISCV::LD || MI.getOpcode() == RISCV::C_LD) {
    Ty = getDefaultPtrType(C);
  }

  RegisterValues[MOp1.getReg()] = Builder.CreateLoad(Ty, Ptr);

  return true;
}

bool RISCV64MachineInstructionRaiser::raiseStoreInstruction(
    const MachineInstr &MI, BasicBlock *BB) {
  IRBuilder<> Builder(BB);

  const MachineOperand &MOp1 = MI.getOperand(0);
  const MachineOperand &MOp2 = MI.getOperand(1);
  const MachineOperand &MOp3 = MI.getOperand(2);

  assert(MOp1.isReg() && MOp3.isImm());

  Value *Val = getRegOrArgValue(MOp1.getReg());

  if (Val == nullptr) {
    printFailure(MI, "Register value of store instruction not set");
    return false;
  }

  Type *Ty = getDefaultIntType(C);
  if (MI.getOpcode() == RISCV::SD || MI.getOpcode() == RISCV::C_SD) {
    Ty = getDefaultPtrType(C);
  }

  Value *Ptr = nullptr;
  if (MOp2.getReg() == RISCV::X8) {
    Ptr = Builder.CreateAlloca(Ty);
    StackValues[MOp3.getImm()] = Ptr;
  } else if (MOp3.getImm() == 0) {
    Ptr = getRegOrArgValue(MOp2.getReg());
  }

  if (Ptr == nullptr) {
    printFailure(MI, "Pointer of store instruction not set");
    return false;
  }

  Builder.CreateStore(Val, Ptr);

  return true;
}

bool RISCV64MachineInstructionRaiser::raiseGlobalInstruction(
    const MachineInstr &MI, BasicBlock *BB) {
  IRBuilder<> Builder(BB);

  const MachineInstr *NextMI = MI.getNextNode();

  if (NextMI->getOpcode() == RISCV::ADDI) {
    const MachineOperand &MOp1 = NextMI->getOperand(0);
    const MachineOperand &MOp3 = NextMI->getOperand(2);

    assert(MOp1.isReg() && MOp3.isImm());

    uint64_t InstOffset = MCIR->getMCInstIndex(MI);
    uint64_t TextOffset = MR->getTextSectionAddress();
    int64_t ValueOffset = MOp3.getImm();

    // First attempt .rodata
    uint64_t RODataOffset = InstOffset + TextOffset + ValueOffset;
    Value *LowerBound = ConstantInt::get(Type::getInt32Ty(C), 0);
    Value *UpperBound = nullptr;
    GlobalVariable *GlobalVar =
        ELFUtils.getRODataValueAtOffset(RODataOffset, UpperBound);

    // Found in .rodata, create getelementptr instruction
    if (GlobalVar != nullptr) {
      RegisterValues[MOp1.getReg()] = Builder.CreateInBoundsGEP(
          GlobalVar->getValueType(), GlobalVar, {LowerBound, UpperBound});
      return true;
    }

    // If not at .rodata, attempt .data
    // TODO: 0x2000 is most likely not correct, but it works?
    uint64_t DataOffset = InstOffset + TextOffset + ValueOffset + 0x2000;
    GlobalVar = ELFUtils.getDataValueAtOffset(DataOffset);

    if (GlobalVar == nullptr) {
      printFailure(MI, "Global value not found");
      return false;
    }

    // Found in .data, create ptrtoint instruction
    RegisterValues[MOp1.getReg()] = GlobalVar;
  } else {
    printFailure(MI, "Not yet implemented AUIPC instruction");
    return false;
  }

  return true;
}

bool RISCV64MachineInstructionRaiser::raiseCallInstruction(
    const MachineInstr &MI, BasicBlock *BB) {
  IRBuilder<> Builder(BB);

  Function *CalledFunction = getCalledFunction(MI);

  if (CalledFunction == nullptr) {
    printFailure(MI, "Called function of call instruction not found");
    return false;
  }

  FunctionType *CalledFunctionType = CalledFunction->getFunctionType();

  // Construct arguments vector based on argument registers
  std::vector<Value *> Args;
  if (CalledFunctionType->isVarArg() ||
      CalledFunctionType->getNumParams() > 0) {
    for (unsigned ArgReg = RISCV::X10; ArgReg < RISCV::X17; ArgReg++) {
      Value *RegVal = RegisterValues[ArgReg];
      if (RegVal == nullptr) {
        break;
      }
      Args.push_back(RegVal);
    }
  }

  // Check if enough arguments are passed
  if (!CalledFunctionType->isVarArg() &&
      Args.size() != CalledFunctionType->getNumParams()) {
    printFailure(MI, "Not enough arguments passed to called function");
    return false;
  }

  // Check if arguments are of correct type
  for (unsigned I = 0; I < CalledFunctionType->getNumParams(); I++) {
    if (Args[I]->getType() != CalledFunctionType->getParamType(I)) {
      printFailure(MI, "Argument type of argument '" + std::to_string(I) +
                           "' does not match prototype");
      return false;
    }
  }

  RegisterValues[RISCV::X10] = Builder.CreateCall(CalledFunction, Args);

  return true;
}

bool RISCV64MachineInstructionRaiser::raiseReturnInstruction(
    const MachineInstr &MI, BasicBlock *BB) {
  IRBuilder<> Builder(BB);

  Type *RetTy = BB->getParent()->getReturnType();

  if (RetTy->isVoidTy()) {
    Builder.CreateRetVoid();
  } else {
    Value *RetVal = RegisterValues[RISCV::X10];

    if (RetVal == nullptr) {
      printFailure(MI, "Register value of return instruction not set");
      return false;
    }

    Builder.CreateRet(RetVal);
  }

  return true;
}

bool RISCV64MachineInstructionRaiser::raiseUnconditonalBranchInstruction(
    const MachineInstr &MI, BasicBlock *BB) {
  IRBuilder<> Builder(BB);

  const MachineOperand &MOp = MI.getOperand(0);

  assert(MOp.isImm());

  BasicBlock *Dest = getBasicBlockAtOffset(MI, MOp.getImm());

  if (Dest == nullptr) {
    printFailure(MI, "A BB has not been created for the specified MBBNo");
    return false;
  }

  Builder.CreateBr(Dest);

  return true;
}

bool RISCV64MachineInstructionRaiser::raiseConditionalBranchInstruction(
    Predicate Pred, const MachineInstr &MI, BasicBlock *BB) {
  IRBuilder<> Builder(BB);

  const MachineOperand &MOp1 = MI.getOperand(0);
  const MachineOperand &MOp2 = MI.getOperand(1);
  const MachineOperand &MOp3 = MI.getOperand(2);

  // the zero register for bnez en beqz is implicit, so no second register
  // operand. All other branch instructions have two register operands and
  // a single immediate operand.
  bool IsImplicitZero =
      MI.getOpcode() == RISCV::C_BNEZ || MI.getOpcode() == RISCV::C_BEQZ;

  Value *RHS = nullptr;
  uint64_t Offset;

  // Offset is either in second or third operand, depending on if the
  // instruction is one of the two zero instructions. The RHS is either
  // a constant 0, or the value of the register of the second operand.
  if (IsImplicitZero) {
    assert(MOp1.isReg() && MOp2.isImm());
    RHS = ConstantInt::get(getDefaultIntType(C), 0);
    Offset = MOp2.getImm();
  } else {
    assert(MOp1.isReg() && MOp2.isReg() && MOp3.isImm());
    RHS = RegisterValues[MOp2.getReg()];
    Offset = MOp3.getImm();
  }

  if (RHS == nullptr) {
    printFailure(MI, "RHS of branch instruction is not set");
    return false;
  }

  Value *LHS = RegisterValues[MOp1.getReg()];
  if (LHS == nullptr) {
    printFailure(MI, "LHS of branch instruction is not set");
    return false;
  }

  // Create compare instruction
  Value *Cond = Builder.CreateCmp(Pred, LHS, RHS);

  // Get successor (fall through branch) basic block
  const MachineBasicBlock *MBB = MI.getParent();
  BasicBlock *FalseBB = BasicBlocks[MBB->getNumber() + 1];
  if (FalseBB == nullptr) {
    printFailure(MI, "The successor BB has not been created.");
    return false;
  }

  // Get destination basic block
  BasicBlock *TrueBB = getBasicBlockAtOffset(MI, Offset);
  if (TrueBB == nullptr) {
    printFailure(MI, "The BB has not been created for the specified MBBNo");
    return false;
  }

  // Create branch instruction
  Builder.CreateCondBr(Cond, TrueBB, FalseBB);

  return true;
}

Function *RISCV64MachineInstructionRaiser::getCalledFunction(
    const MachineInstr &MI) const {
  const MachineOperand &MOp2 = MI.getOperand(1);

  assert(MOp2.isImm());

  Function *CalledFunction = nullptr;

  // Calculate offset of function
  uint64_t InstructionOffset = MCIR->getMCInstIndex(MI);
  int64_t TextSectionOffset = MR->getTextSectionAddress();
  uint64_t Offset = InstructionOffset + MOp2.getImm() + TextSectionOffset;

  // First check if function is part of executable
  CalledFunction = MR->getRaisedFunctionAt(Offset);

  // If not, use PLT section
  if (CalledFunction == nullptr) {
    CalledFunction = ELFUtils.getFunctionAtOffset(Offset);
  }

  return CalledFunction;
}

BasicBlock *
RISCV64MachineInstructionRaiser::getBasicBlockAtOffset(const MachineInstr &MI,
                                                       uint64_t Offset) {
  uint64_t InstructionOffset = MCIR->getMCInstIndex(MI) + Offset;

  // Get number of the MBB of instruction at the offset
  int64_t MBBNo = MCIR->getMBBNumberOfMCInstOffset(InstructionOffset, MF);

  if (MBBNo == -1) {
    printFailure(MI, "No MBB maps to the specified offset");
    return nullptr;
  }

  return BasicBlocks[MBBNo];
}
