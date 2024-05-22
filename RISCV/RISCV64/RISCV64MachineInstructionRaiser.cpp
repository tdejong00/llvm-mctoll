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
#include "MachineInstructionRaiser.h"
#include "ModuleRaiser.h"
#include "RISCV64FunctionPrototypeDiscoverer.h"
#include "RISCV64MachineInstructionRaiserUtils.h"
#include "RISCV64ValueTracker.h"
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
#include "llvm/IR/Operator.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Value.h"
#include "llvm/MC/MCInst.h"
#include "llvm/Object/SymbolicFile.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/MachineValueType.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <netinet/in.h>
#include <string>
#include <sys/types.h>
#include <vector>
#include <zconf.h>

#define DEBUG_TYPE "mctoll"

using namespace llvm;
using namespace mctoll;
using namespace RISCV;
using namespace RISCV64MachineInstructionRaiserUtils;

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
      MCIR(MCIR), FunctionPrototypeDiscoverer(MF), ValueTracker(this),
      ELFUtils(MR, C) {
  Zero = ConstantInt::get(Type::getInt64Ty(C), 0);
}

bool RISCV64MachineInstructionRaiser::raise() {
  LoopTraversal Traversal;
  LoopTraversal::TraversalOrder TraversalOrder = Traversal.traverse(MF);

  // Traverse basic blocks of machine function and raise all non-terminators
  for (LoopTraversal::TraversedMBBInfo MBBInfo : TraversalOrder) {
    const MachineBasicBlock *MBB = MBBInfo.MBB;

    // Only process basic blocks once
    if (!MBBInfo.PrimaryPass) {
      continue;
    }

    // Initialize hardwired zero register
    ValueTracker.setRegValue(MBB->getNumber(), X0, Zero);

    // Create basic block
    BasicBlock *BB = BasicBlock::Create(C, "", RaisedFunction);

    // Store basic block for future reference
    BasicBlocks[MBB->getNumber()] = BB;

    // Loop over machine instructions of basic block and raise each instruction.
    for (const MachineInstr &MI : MBB->instrs()) {
      bool WasAUIPC = MI.getPrevNode() != nullptr &&
                      MI.getPrevNode()->getOpcode() == AUIPC;

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

      // Skip raising terminator instructions, record
      // register values for use in second pass
      if (MI.isTerminator() &&
          getInstructionType(MI.getOpcode()) != InstructionType::RETURN) {
        printSkipped(MI, "Skipped raising terminator instruction");

        // Record information about terminator instruction for use in later pass
        ControlTransferInfo *Info = new ControlTransferInfo;
        Info->CandidateBlock = BB;
        Info->CandidateMachineInstr = &MI;
        for (unsigned int I = 0; I < MI.getNumOperands(); I++) {
          const MachineOperand &MOp = MI.getOperand(I);
          if (MOp.isReg()) {
            Value *RegValue = getRegOrArgValue(MOp.getReg(), MBB->getNumber());
            Info->RegValues.push_back(RegValue);
          } else {
            Info->RegValues.push_back(nullptr);
          }
        }
        CTInfo.push_back(Info);

        continue;
      }

      // Raise non-terminator instruction
      if (raiseNonTerminatorInstruction(MI, MBB->getNumber())) {
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

      // Find recorded info
      auto It = std::find_if(CTInfo.begin(), CTInfo.end(),
                             [&MI](ControlTransferInfo *Info) {
                               return Info->CandidateMachineInstr == &MI;
                             });
      assert(It != CTInfo.end() && "Control Transfer Info not recorded");

      if (raiseTerminatorInstruction(*It)) {
        printSuccess(MI);
      }

      CTInfo.erase(It);
    }
  }

  // All recorded instructions should have been handled
  assert(CTInfo.empty() && "Unhandled branch instructions");

  LLVM_DEBUG(dbgs() << "\nAfter raising terminator instructions:\n");
  LLVM_DEBUG(RaisedFunction->dump());

  return true;
}

FunctionType *RISCV64MachineInstructionRaiser::getRaisedFunctionPrototype() {
  RaisedFunction = FunctionPrototypeDiscoverer.discoverFunctionPrototype();
  return RaisedFunction->getFunctionType();
}

int RISCV64MachineInstructionRaiser::getArgumentNumber(unsigned int PReg) {
  unsigned int ArgReg = PReg - X10;
  if (ArgReg < 8) {
    return ArgReg;
  }
  return -1;
}

Value *RISCV64MachineInstructionRaiser::getRegOrArgValue(unsigned int PReg,
                                                         int MBBNo) {
  Value *Val = ValueTracker.getRegValue(MBBNo, PReg);

  int ArgNo = getArgumentNumber(PReg);
  int NumArgs = RaisedFunction->getFunctionType()->getNumParams();

  // Attempt to get value from function arguments
  if (Val == nullptr && ArgNo >= 0 && ArgNo < NumArgs) {
    Val = RaisedFunction->getArg(ArgNo);
  }

  return Val;
}

Value *
RISCV64MachineInstructionRaiser::getRegOrImmValue(const MachineOperand &MOp,
                                                  int MBBNo) {
  assert(MOp.isReg() || MOp.isImm());

  if (MOp.isReg()) {
    return getRegOrArgValue(MOp.getReg(), MBBNo);
  }

  return ConstantInt::get(getDefaultIntType(C), MOp.getImm());
}

BasicBlock *RISCV64MachineInstructionRaiser::getBasicBlock(int MBBNo) {
  return BasicBlocks[MBBNo];
}

bool RISCV64MachineInstructionRaiser::coerceType(Value *&Val, Type *DestTy,
                                                 IRBuilder<> &Builder) {
  Type *ValTy = Val->getType();

  // No coercion needed when types are the same
  if (ValTy == DestTy) {
    return true;
  }

  // For integer types, truncate when bigger and a sign-extend when smaller.
  if (ValTy->isIntegerTy() && DestTy->isIntegerTy()) {
    if (ValTy->getIntegerBitWidth() > DestTy->getIntegerBitWidth()) {
      Val = Builder.CreateTrunc(Val, DestTy);
    } else {
      Val = Builder.CreateSExt(Val, DestTy);
    }
  }
  // When destination type is a pointer an the current type is an integer
  // value of zero, convert to nullptr
  else if (DestTy->isPointerTy() && isa<ConstantInt>(Val) &&
           dyn_cast<ConstantInt>(Val)->getValue().isZero()) {
    Val = ConstantPointerNull::get(dyn_cast<PointerType>(DestTy));
  }
  // When destination type is a pointer and the current
  // type is an i64 create a IntToPtr instruction
  else if (ValTy == Type::getInt64Ty(C) && DestTy->isPointerTy()) {
    Val = Builder.CreateIntToPtr(Val, DestTy);
  }
  // When destination type is an i64 and the current
  // type is a pointer create a PtrToInt instruction
  else if (ValTy->isPointerTy() && DestTy == Type::getInt64Ty(C)) {
    Val = Builder.CreatePtrToInt(Val, DestTy);
  }
  // No coercion possible
  else {
    return false;
  }

  return true;
}

bool RISCV64MachineInstructionRaiser::widenType(Value *&LHS, Value *&RHS,
                                                IRBuilder<> &Builder) {
  Type *LTy = LHS->getType();
  Type *RTy = RHS->getType();

  // No widening needed when types are the same
  if (LTy == RTy) {
    return true;
  }

  // Widening not defined for non-integer types
  if (!LTy->isIntegerTy() || !RTy->isIntegerTy()) {
    return false;
  }

  // LHS is bigger than RHS, coerce type of LHS via sign extension
  if (LTy->getIntegerBitWidth() > RTy->getIntegerBitWidth()) {
    RHS = Builder.CreateSExt(RHS, LTy);
  }
  // RHS is bigger than LHS, coerce type of RHS via sign extension
  else if (LTy->getIntegerBitWidth() < RTy->getIntegerBitWidth()) {
    LHS = Builder.CreateSExt(LHS, RTy);
  }
  // No widening possible
  else {
    return false;
  }

  return true;
}

bool RISCV64MachineInstructionRaiser::raiseNonTerminatorInstruction(
    const MachineInstr &MI, int MBBNo) {
  InstructionType Type = getInstructionType(MI.getOpcode());

  assert(Type != InstructionType::UNCONDITIONAL_BRANCH &&
         Type != InstructionType::CONDITIONAL_BRANCH);

  if (Type == InstructionType::BINOP) {
    BinaryOps BinOp = toBinaryOperation(MI.getOpcode());

    if (BinOp == BinaryOps::BinaryOpsEnd) {
      printFailure(MI, "Unimplemented or unknown binary operation");
      return false;
    }

    return raiseBinaryOperation(BinOp, MI, MBBNo);
  }

  switch (Type) {
  case InstructionType::NOP:
    return true;
  case InstructionType::MOVE:
    return raiseMoveInstruction(MI, MBBNo);
  case InstructionType::LOAD:
    return raiseLoadInstruction(MI, MBBNo);
  case InstructionType::STORE:
    return raiseStoreInstruction(MI, MBBNo);
  case InstructionType::GLOBAL:
    return raiseGlobalInstruction(MI, MBBNo);
  case InstructionType::CALL:
    return raiseCallInstruction(MI, MBBNo);
  case InstructionType::RETURN:
    return raiseReturnInstruction(MI, MBBNo);
  default:
    printFailure(MI, "Unimplemented or unknown instruction");
    return false;
  }
}

bool RISCV64MachineInstructionRaiser::raiseTerminatorInstruction(
    ControlTransferInfo *Info) {
  const MachineInstr *MI = Info->CandidateMachineInstr;
  InstructionType Type = getInstructionType(MI->getOpcode());

  assert(Type == InstructionType::UNCONDITIONAL_BRANCH ||
         Type == InstructionType::CONDITIONAL_BRANCH);

  switch (Type) {
  case InstructionType::UNCONDITIONAL_BRANCH:
    return raiseUnconditonalBranchInstruction(Info);
  case InstructionType::CONDITIONAL_BRANCH: {
    Predicate Pred = toPredicate(MI->getOpcode());
    return raiseConditionalBranchInstruction(Pred, Info);
  }
  default:
    printFailure(*MI, "Unimplemented or unknown instruction");
    return false;
  }
}

bool RISCV64MachineInstructionRaiser::raiseBinaryOperation(
    BinaryOps BinOp, const MachineInstr &MI, int MBBNo) {
  BasicBlock *BB = getBasicBlock(MBBNo);
  IRBuilder<> Builder(BB);

  const MachineOperand &MOp1 = MI.getOperand(0);
  const MachineOperand &MOp2 = MI.getOperand(1);
  const MachineOperand &MOp3 = MI.getOperand(2);

  assert(MOp1.isReg() && MOp2.isReg() && (MOp3.isReg() || MOp3.isImm()));

  // Instructions like `addi s0,$` should load value at stack offset
  if (BinOp == BinaryOps::Add && MOp2.getReg() == X8 && MOp3.isImm()) {
    Value *StackValue = ValueTracker.getStackSlot(MOp3.getImm());
    ValueTracker.setRegValue(MBBNo, MOp1.getReg(), StackValue);
    return true;
  }

  Value *LHS = getRegOrArgValue(MOp2.getReg(), MBBNo);
  if (LHS == nullptr) {
    printFailure(MI, "LHS value of add instruction not set");
    return false;
  }

  Value *RHS = getRegOrImmValue(MOp3, MBBNo);
  if (RHS == nullptr) {
    printFailure(MI, "RHS value of add instruction not set");
    return false;
  }

  Type *LTy = LHS->getType();
  Type *RTy = RHS->getType();

  // Instructions where the lhs is a pointer and the rhs is not (or vice
  // versa) calculate an address. This will be raised using a GEP instruction.
  if (BinOp == BinaryOps::Add && LTy->isPointerTy() != RTy->isPointerTy()) {
    Value *Ptr = LTy->isPointerTy() ? LHS : RHS;
    Value *Val = LTy->isPointerTy() ? RHS : LHS;
    return raiseAddressOffsetInstruction(MI, Ptr, Val, MBBNo);
  }

  // Type mismatch
  if (!widenType(LHS, RHS, Builder)) {
    printFailure(MI, "Type mismatch for binary operation");
    LHS->dump();
    RHS->dump();
    return false;
  }

  Value *Val = Builder.CreateBinOp(BinOp, LHS, RHS);
  ValueTracker.setRegValue(MBBNo, MOp1.getReg(), Val);

  return true;
}

bool RISCV64MachineInstructionRaiser::raiseAddressOffsetInstruction(
    const MachineInstr &MI, Value *Ptr, Value *Val, int MBBNo) {
  BasicBlock *BB = getBasicBlock(MBBNo);
  IRBuilder<> Builder(BB);

  assert(Ptr->getType()->isPointerTy() && "expected a pointer type");
  assert(!Val->getType()->isPointerTy() && "expected an integral type");

  const MachineOperand &MOp1 = MI.getOperand(0);
  assert(MOp1.isReg());

  // Remove unnecessary Shl instructions for array accesses, because the
  // operands of the GEP instructions represent indices and not number of
  // bytes.
  if (BinaryOperator *BinOp = dyn_cast<BinaryOperator>(Val)) {
    Value *LHS = BinOp->getOperand(0);
    Value *RHS = BinOp->getOperand(1);
    if (BinOp->getOpcode() == BinaryOps::Shl && isa<ConstantInt>(RHS)) {
      Val = LHS;
      BinOp->eraseFromParent();
    }
  }

  // Create GEP instruction
  Value *GEP = nullptr;
  if (GEPOperator *GEPOp = dyn_cast<GEPOperator>(Ptr)) {
    Type *Ty = GEPOp->getResultElementType();
    GEP = Builder.CreateInBoundsGEP(Ty, GEPOp, Val);
  } else if (GlobalVariable *GlobalVar = dyn_cast<GlobalVariable>(Ptr)) {
    Type *Ty = getDefaultType(C, *MI.getNextNode());
    GEP = Builder.CreateInBoundsGEP(Ty, GlobalVar, Val);
  } else if (LoadInst *Load = dyn_cast<LoadInst>(Ptr)) {
    Type *Ty = getDefaultType(C, *MI.getNextNode());
    GEP = Builder.CreateInBoundsGEP(Ty, Load, Val);
  } else {
    printFailure(MI, "unexpected pointer type");
    return false;
  }

  ValueTracker.setRegValue(MBBNo, MOp1.getReg(), GEP);

  return true;
}

bool RISCV64MachineInstructionRaiser::raiseMoveInstruction(
    const MachineInstr &MI, int MBBNo) {
  BasicBlock *BB = getBasicBlock(MBBNo);
  IRBuilder<> Builder(BB);

  const MachineOperand &MOp1 = MI.getOperand(0);
  const MachineOperand &MOp2 = MI.getOperand(1);

  assert(MOp1.isReg() && (MOp2.isReg() || MOp2.isImm()));

  Value *Val = getRegOrImmValue(MOp2, MBBNo);

  if (Val == nullptr) {
    printFailure(MI, "Register value of move instruction not set");
    return false;
  }

  ValueTracker.setRegValue(MBBNo, MOp1.getReg(), Val);

  return true;
}

bool RISCV64MachineInstructionRaiser::raiseLoadInstruction(
    const MachineInstr &MI, int MBBNo) {
  BasicBlock *BB = getBasicBlock(MBBNo);
  IRBuilder<> Builder(BB);

  const MachineOperand &MOp1 = MI.getOperand(0);
  const MachineOperand &MOp2 = MI.getOperand(1);
  const MachineOperand &MOp3 = MI.getOperand(2);

  assert(MOp1.isReg() && MOp2.isReg() && MOp3.isImm());

  Value *Ptr = nullptr;

  // Load from stack
  if (MOp2.getReg() == X8) {
    int64_t StackOffset = MOp3.getImm();
    Ptr = ValueTracker.getStackSlot(StackOffset);
  }
  // Load from address specified in register
  else if (MOp3.getImm() == 0) {
    Ptr = getRegOrArgValue(MOp2.getReg(), MBBNo);
  }
  // Load from array
  else {
    Value *ArrayPtr = getRegOrArgValue(MOp2.getReg(), MBBNo);
    if (ArrayPtr == nullptr) {
      printFailure(MI, "Array pointer of store instruction not set");
      return false;
    }

    // Determine type for GEP
    if (GEPOperator *GEPOp = dyn_cast<GEPOperator>(ArrayPtr)) {
      Type *Ty = GEPOp->getSourceElementType();
      ConstantInt *Index =
          toGEPIndex(C, MOp3.getImm(), getAlign(MI.getOpcode()));
      Ptr = Builder.CreateInBoundsGEP(Ty, ArrayPtr, Index);
    } else if (GlobalVariable *GlobalVar = dyn_cast<GlobalVariable>(ArrayPtr)) {
      Type *Ty = GlobalVar->getValueType();
      ConstantInt *Index =
          toGEPIndex(C, MOp3.getImm(), getAlign(MI.getOpcode()));
      Ptr = Builder.CreateInBoundsGEP(Ty, ArrayPtr, {Zero, Index});
    } else if (LoadInst *Load = dyn_cast<LoadInst>(ArrayPtr)) {
      Type *Ty = getDefaultType(C, MI);
      ConstantInt *Index =
          toGEPIndex(C, MOp3.getImm(), getAlign(MI.getOpcode()));
      Value *IntToPtr = Builder.CreateIntToPtr(Load, getDefaultPtrType(C));
      Ptr = Builder.CreateInBoundsGEP(Ty, IntToPtr, Index);
    } else {
      Ptr = ArrayPtr;
    }
  }

  if (Ptr == nullptr) {
    printFailure(MI, "Pointer of load instruction not set");
    return false;
  }

  Type *Ty = getDefaultType(C, MI);

  // When loading from stack, use the allocated type
  if (MOp2.getReg() == X8) {
    AllocaInst *Alloca = dyn_cast<AllocaInst>(Ptr);
    Ty = Alloca->getAllocatedType();
  }

  // Load instructions require an actual pointer, cast i64
  // to ptr and offset the address using a GEP instruction.
  if (Ptr->getType() == Type::getInt64Ty(C)) {
    Type *Ty = getDefaultType(C, MI);
    ConstantInt *Index = toGEPIndex(C, MOp3.getImm(), getAlign(MI.getOpcode()));
    Value *IntToPtr = Builder.CreateIntToPtr(Ptr, getDefaultPtrType(C));
    Ptr = Builder.CreateInBoundsGEP(Ty, IntToPtr, Index);
  }

  LoadInst *Load = Builder.CreateLoad(Ty, Ptr);
  ValueTracker.setRegValue(MBBNo, MOp1.getReg(), Load);

  return true;
}

bool RISCV64MachineInstructionRaiser::raiseStoreInstruction(
    const MachineInstr &MI, int MBBNo) {
  BasicBlock *BB = getBasicBlock(MBBNo);
  IRBuilder<> Builder(BB);

  const MachineOperand &MOp1 = MI.getOperand(0);
  const MachineOperand &MOp2 = MI.getOperand(1);
  const MachineOperand &MOp3 = MI.getOperand(2);

  assert(MOp1.isReg() && MOp3.isImm());

  Value *Val = getRegOrArgValue(MOp1.getReg(), MBBNo);

  if (Val == nullptr) {
    printFailure(MI, "Register value of store instruction not set");
    return false;
  }

  Value *Ptr = nullptr;
  // Store to stack
  if (MOp2.getReg() == X8) {
    Ptr = ValueTracker.getStackSlot(MOp3.getImm(), Val->getType());
  }
  // Store to address specified in register
  else if (MOp3.getImm() == 0) {
    Ptr = getRegOrArgValue(MOp2.getReg(), MBBNo);
  }
  // Store to array
  else {
    Value *ArrayPtr = getRegOrArgValue(MOp2.getReg(), MBBNo);
    if (ArrayPtr == nullptr) {
      printFailure(MI, "Array pointer of store instruction not set");
      return false;
    }

    // Determine type for GEP
    if (GEPOperator *GEPOp = dyn_cast<GEPOperator>(ArrayPtr)) {
      Type *Ty = GEPOp->getSourceElementType();
      ConstantInt *Index =
          toGEPIndex(C, MOp3.getImm(), getAlign(MI.getOpcode()));
      Ptr = Builder.CreateInBoundsGEP(Ty, GEPOp, Index);
    } else if (GlobalVariable *GlobalVar = dyn_cast<GlobalVariable>(ArrayPtr)) {
      Type *Ty = GlobalVar->getValueType();
      ConstantInt *Index =
          toGEPIndex(C, MOp3.getImm(), getAlign(MI.getOpcode()));
      Ptr = Builder.CreateInBoundsGEP(Ty, GlobalVar, {Zero, Index});
    } else if (LoadInst *Load = dyn_cast<LoadInst>(ArrayPtr)) {
      Type *Ty = Load->getPointerOperandType();
      // If next instruction is load, determine type from that load instruction
      const MachineInstr *NextMI = MI.getNextNode();
      if (NextMI != nullptr &&
          getInstructionType(NextMI->getOpcode()) == InstructionType::LOAD) {
        Ty = getDefaultType(C, *NextMI);
      }

      ConstantInt *Index =
          toGEPIndex(C, MOp3.getImm(), getAlign(MI.getOpcode()));
      Value *IntToPtr = Builder.CreateIntToPtr(Load, getDefaultPtrType(C));
      Ptr = Builder.CreateInBoundsGEP(Ty, IntToPtr, Index);
    } else {
      Ptr = ArrayPtr;
    }
  }

  if (Ptr == nullptr) {
    printFailure(MI, "Pointer of store instruction not set");
    return false;
  }

  // Store instructions require an actual pointer, cast i64 to ptr
  if (Ptr->getType() == Type::getInt64Ty(C)) {
    Ptr = Builder.CreateIntToPtr(Ptr, getDefaultPtrType(C));
  }

  Builder.CreateStore(Val, Ptr);

  return true;
}

bool RISCV64MachineInstructionRaiser::raiseGlobalInstruction(
    const MachineInstr &MI, int MBBNo) {
  BasicBlock *BB = getBasicBlock(MBBNo);
  IRBuilder<> Builder(BB);

  const MachineInstr *NextMI = MI.getNextNode();

  if (NextMI->getOpcode() != ADDI && NextMI->getOpcode() != LD) {
    printFailure(MI, "Expected instruction after AUIPC to be ADDI or LD");
    return false;
  }

  const MachineOperand &AUIPCMOp2 = MI.getOperand(1);
  const MachineOperand &NextMOp1 = NextMI->getOperand(0);
  const MachineOperand &NextMOp3 = NextMI->getOperand(2);

  assert(AUIPCMOp2.isImm() && NextMOp1.isReg() && NextMOp3.isImm());

  // AUIPC offset is shifted left by 12 bits
  uint64_t PCOffset = AUIPCMOp2.getImm() << 12;

  // Determine offset
  uint64_t InstOffset = MCIR->getMCInstIndex(MI);
  uint64_t TextOffset = MR->getTextSectionAddress();
  int64_t ValueOffset = NextMOp3.getImm();

  uint64_t Offset = InstOffset + TextOffset + ValueOffset;
  if (NextMI->getOpcode() == LD) {
    Offset += PCOffset;
  }

  // First attempt dynamic relocation
  GlobalVariable *GlobalVar = ELFUtils.getDynRelocValueAtOffset(Offset);
  if (GlobalVar != nullptr) {
    ValueTracker.setRegValue(MBBNo, NextMOp1.getReg(), GlobalVar);
    return true;
  }

  // First attempt .rodata
  Value *Index = nullptr;
  GlobalVar = ELFUtils.getRODataValueAtOffset(Offset, Index);

  // Found in .rodata, create getelementptr instruction
  if (GlobalVar != nullptr) {
    Type *Ty = GlobalVar->getValueType();
    Value *GEP = Builder.CreateInBoundsGEP(Ty, GlobalVar, {Zero, Index});
    ValueTracker.setRegValue(MBBNo, NextMOp1.getReg(), GEP);
    return true;
  }

  // If not at .rodata, attempt .data
  uint64_t DataOffset = InstOffset + TextOffset + ValueOffset + PCOffset;
  GlobalVar = ELFUtils.getDataValueAtOffset(DataOffset);

  if (GlobalVar == nullptr) {
    printFailure(MI, "Global value not found");
    return false;
  }

  // Found in .data
  ValueTracker.setRegValue(MBBNo, NextMOp1.getReg(), GlobalVar);

  return true;
}

bool RISCV64MachineInstructionRaiser::raiseCallInstruction(
    const MachineInstr &MI, int MBBNo) {
  BasicBlock *BB = getBasicBlock(MBBNo);
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
    for (unsigned int ArgReg = X10; ArgReg < X17; ArgReg++) {
      // Do not add too many arguments, values might
      // still be present from  previous function calls.
      if (Args.size() == CalledFunctionType->getNumParams() &&
          !CalledFunctionType->isVarArg()) {
        break;
      }

      Value *RegVal = getRegOrArgValue(ArgReg, MBBNo);
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
  for (unsigned int I = 0; I < CalledFunctionType->getNumParams(); I++) {
    Type *ArgTy = Args[I]->getType();
    Type *ParamTy = CalledFunctionType->getFunctionParamType(I);

    // Type mismatch
    if (!coerceType(Args[I], ParamTy, Builder)) {
      printFailure(MI, "Type mismatch for call instruction");
      ArgTy->dump();
      ParamTy->dump();
      return false;
    }
  }

  CallInst *Call = Builder.CreateCall(CalledFunction, Args);
  ValueTracker.setRegValue(MBBNo, X10, Call);

  return true;
}

bool RISCV64MachineInstructionRaiser::raiseReturnInstruction(
    const MachineInstr &MI, int MBBNo) {
  BasicBlock *BB = getBasicBlock(MBBNo);
  IRBuilder<> Builder(BB);

  Type *RetTy = BB->getParent()->getReturnType();

  if (RetTy->isVoidTy()) {
    Builder.CreateRetVoid();
  } else {
    Value *RetVal = getRegOrArgValue(X10, MBBNo);

    if (RetVal == nullptr) {
      printFailure(MI, "Register value of return instruction not set");
      return false;
    }

    // Type mismatch
    if (!coerceType(RetVal, RetTy, Builder)) {
      printFailure(MI, "Type mismatch for return instruction");
      RetVal->getType()->dump();
      RetTy->dump();
      return false;
    }

    Builder.CreateRet(RetVal);
  }

  return true;
}

bool RISCV64MachineInstructionRaiser::raiseUnconditonalBranchInstruction(
    ControlTransferInfo *Info) {
  IRBuilder<> Builder(Info->CandidateBlock);

  const MachineInstr &MI = *Info->CandidateMachineInstr;

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
    Predicate Pred, ControlTransferInfo *Info) {
  IRBuilder<> Builder(Info->CandidateBlock);

  const MachineInstr &MI = *Info->CandidateMachineInstr;

  const MachineOperand &MOp1 = MI.getOperand(0);
  const MachineOperand &MOp2 = MI.getOperand(1);
  const MachineOperand &MOp3 = MI.getOperand(2);

  // The zero register for bnez en beqz is implicit, so no second register
  // operand. All other branch instructions have two register operands and
  // a single immediate operand.
  bool IsImplicitZero =
      MI.getOpcode() == C_BNEZ || MI.getOpcode() == C_BEQZ;

  Value *RHS = nullptr;
  uint64_t Offset;

  // Offset is either in second or third operand, depending on if the
  // instruction is one of the two zero instructions. The RHS is either
  // a constant 0, or the value of the register of the second operand.
  if (IsImplicitZero) {
    assert(MOp1.isReg() && MOp2.isImm());
    RHS = Zero;
    Offset = MOp2.getImm();
  } else {
    assert(MOp1.isReg() && MOp2.isReg() && MOp3.isImm());
    RHS = Info->RegValues[1];
    Offset = MOp3.getImm();
  }

  if (RHS == nullptr) {
    printFailure(MI, "RHS of branch instruction is not set");
    return false;
  }

  Value *LHS = Info->RegValues[0];
  if (LHS == nullptr) {
    printFailure(MI, "LHS of branch instruction is not set");
    return false;
  }

  // Convert RHS to nullptr when comparison with zero when LHS is a pointer
  if (LHS->getType()->isPointerTy() && isa<ConstantInt>(RHS) &&
      dyn_cast<ConstantInt>(RHS)->getValue().isZero()) {
    RHS = ConstantPointerNull::get(dyn_cast<PointerType>(LHS->getType()));
  }

  // Type mismatch
  if (!widenType(LHS, RHS, Builder)) {
    printFailure(MI, "Type mismatch for comparison operation");
    LHS->dump();
    RHS->dump();
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

  return getBasicBlock(MBBNo);
}
