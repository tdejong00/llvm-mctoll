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
      // Skip prolog instructions
      if (isPrologInstruction(MI)) {
        printSkipped(MI, "Skipped raising of prolog instruction");
        continue;
      }

      // Skip epilog instructions
      if (isEpilogInstruction(MI)) {
        printSkipped(MI, "Skipped raising of epilog instruction");
        continue;
      }

      // The instruction after an AUIPC is already handled
      //  during raising of the AUIPC instruction.
      if (MI.getPrevNode() != nullptr &&
          MI.getPrevNode()->getOpcode() == AUIPC) {
        printSkipped(MI, "Already raised by previous instruction");
        continue;
      }

      // Skip raising terminator instructions, record
      // register values for use in second pass
      if (MI.isTerminator() && MI.getOpcode() != C_JR) {
        printSkipped(MI, "Skipped raising terminator instruction");

        // Record information about terminator instruction for use in later pass
        ControlTransferInfo *Info = new ControlTransferInfo;
        Info->CandidateBlock = BB;
        Info->CandidateMachineInstr = &MI;
        for (unsigned int I = 0; I < MI.getNumOperands(); I++) {
          const MachineOperand &MOp = MI.getOperand(I);
          Value *RegValue = nullptr;
          if (MOp.isReg()) {
            RegValue = getRegOrArgValue(MOp.getReg(), MBB->getNumber());
          }
          Info->RegValues.push_back(RegValue);
        }
        CTInfo.push_back(Info);

        continue;
      }

      // Raise non-terminator instruction
      if (raiseNonTerminator(MI, MBB->getNumber())) {
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

      if (raiseTerminator(*It)) {
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

  return ConstantInt::get(Type::getInt32Ty(C), MOp.getImm());
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

bool RISCV64MachineInstructionRaiser::raiseNonTerminator(const MachineInstr &MI,
                                                         int MBBNo) {
  unsigned int Op = MI.getOpcode();

  BinaryOps BinOp = toBinaryOperation(Op);
  if (BinOp != BinaryOps::BinaryOpsEnd) {
    return raiseBinaryOperation(BinOp, MI, MBBNo);
  }

  switch (Op) {
  case C_NOP:
    return true;
  case C_MV:
  case C_LI:
    return raiseMove(MI, MBBNo);
  case LB:
  case LBU:
  case LH:
  case LHU:
  case LW:
  case LWU:
  case LD:
  case C_LW:
  case C_LD:
    return raiseLoad(MI, MBBNo);
  case SB:
  case SH:
  case SW:
  case SD:
  case C_SW:
  case C_SD:
    return raiseStore(MI, MBBNo);
  case AUIPC:
  case LUI:
  case C_LUI:
    return raisePCRelativeAccess(MI, MBBNo);
  case JAL:
  case C_JAL:
    return raiseCall(MI, MBBNo);
  case C_JR:
  case JALR:
  case C_JALR:
    return raiseReturn(MI, MBBNo);
  default:
    printFailure(MI, "Unimplemented or unknown non-terminator instruction");
    return false;
  }
}

bool RISCV64MachineInstructionRaiser::raiseTerminator(
    ControlTransferInfo *Info) {
  const MachineInstr *MI = Info->CandidateMachineInstr;
  unsigned int Op = MI->getOpcode();

  if (Op == C_J) {
    return raiseUnconditonalBranch(Info);
  }

  Predicate Pred = toPredicate(Op);

  if (Pred != Predicate::BAD_ICMP_PREDICATE) {
    return raiseConditionalBranch(Pred, Info);
  }

  printFailure(*MI, "Unimplemented or unknown terminator instruction");
  return false;
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

  // Get left-hand side value
  Value *LHS = getRegOrArgValue(MOp2.getReg(), MBBNo);
  if (LHS == nullptr) {
    printFailure(MI, "LHS value of binary operation not set");
    return false;
  }

  // Get right-hand side value
  Value *RHS = getRegOrImmValue(MOp3, MBBNo);
  if (RHS == nullptr) {
    printFailure(MI, "RHS value of binary operation not set");
    return false;
  }

  // Convert pointers to i64 for binary operation
  if (LHS->getType()->isPointerTy()) {
    LHS = Builder.CreatePtrToInt(LHS, Type::getInt64Ty(C));
  }
  if (RHS->getType()->isPointerTy()) {
    RHS = Builder.CreatePtrToInt(RHS, Type::getInt64Ty(C));
  }

  // Type mismatch
  if (!widenType(LHS, RHS, Builder)) {
    printFailure(MI, "Type mismatch for binary operation");
    LHS->dump();
    RHS->dump();
    return false;
  }

  // Create binary operation instruction and assign to destination register
  Value *Val = Builder.CreateBinOp(BinOp, LHS, RHS);
  ValueTracker.setRegValue(MBBNo, MOp1.getReg(), Val);

  return true;
}

bool RISCV64MachineInstructionRaiser::raiseMove(const MachineInstr &MI,
                                                int MBBNo) {
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

  // Assign value to destination register
  ValueTracker.setRegValue(MBBNo, MOp1.getReg(), Val);

  return true;
}

bool RISCV64MachineInstructionRaiser::raiseLoad(const MachineInstr &MI,
                                                int MBBNo) {
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
  // Load from array or pointer
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
          toGEPIndex(C, MOp3.getImm(), GlobalVar->getAlign()->value());
      Ptr = Builder.CreateInBoundsGEP(Ty, ArrayPtr, {Zero, Index});
    } else if (LoadInst *Load = dyn_cast<LoadInst>(ArrayPtr)) {
      Type *Ty = getType(C, MI.getOpcode());
      ConstantInt *Index =
          toGEPIndex(C, MOp3.getImm(), getAlign(MI.getOpcode()));
      Value *IntToPtr = Builder.CreateIntToPtr(Load, Type::getInt64PtrTy(C));
      Ptr = Builder.CreateInBoundsGEP(Ty, IntToPtr, Index);
    } else {
      Ptr = ArrayPtr;
    }
  }

  if (Ptr == nullptr) {
    printFailure(MI, "Pointer of load instruction not set");
    return false;
  }

  Type *Ty = getType(C, MI.getOpcode());

  // When loading from stack, use the allocated type
  if (MOp2.getReg() == X8) {
    AllocaInst *Alloca = dyn_cast<AllocaInst>(Ptr);
    Ty = Alloca->getAllocatedType();
  }

  // Load instructions require an actual pointer, cast i64
  // to ptr and offset the address using a GEP instruction.
  if (Ptr->getType() == Type::getInt64Ty(C)) {
    ConstantInt *Index = toGEPIndex(C, MOp3.getImm(), getAlign(MI.getOpcode()));
    Value *IntToPtr = Builder.CreateIntToPtr(Ptr, Type::getInt64PtrTy(C));
    Ptr = Builder.CreateInBoundsGEP(Ty, IntToPtr, Index);
  }

  // Create load instruction and assign to destination register
  LoadInst *Load = Builder.CreateLoad(Ty, Ptr);
  ValueTracker.setRegValue(MBBNo, MOp1.getReg(), Load);

  return true;
}

bool RISCV64MachineInstructionRaiser::raiseStore(const MachineInstr &MI,
                                                 int MBBNo) {
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
          toGEPIndex(C, MOp3.getImm(), GlobalVar->getAlign()->value());
      Ptr = Builder.CreateInBoundsGEP(Ty, GlobalVar, {Zero, Index});
    } else if (LoadInst *Load = dyn_cast<LoadInst>(ArrayPtr)) {
      Type *Ty = Load->getPointerOperandType();
      // If next instruction is load, determine type from that load instruction
      const MachineInstr *NextMI = MI.getNextNode();
      if (NextMI != nullptr && isLoad(NextMI->getOpcode())) {
        Ty = getType(C, NextMI->getOpcode());
      }

      ConstantInt *Index =
          toGEPIndex(C, MOp3.getImm(), getAlign(MI.getOpcode()));
      Value *IntToPtr = Builder.CreateIntToPtr(Load, Type::getInt64PtrTy(C));
      Ptr = Builder.CreateInBoundsGEP(Ty, IntToPtr, Index);
    } else {
      Ptr = ArrayPtr;
    }
  }

  if (Ptr == nullptr) {
    printFailure(MI, "Pointer of store instruction not set");
    return false;
  }

  // Store instructions require an actual pointer, cast i64 to ptr and offset
  // the address using a GEP instruction.
  if (Ptr->getType() == Type::getInt64Ty(C)) {
    Type *Ty = getType(C, MI.getOpcode());
    ConstantInt *Index = toGEPIndex(C, MOp3.getImm(), getAlign(MI.getOpcode()));
    Value *IntToPtr = Builder.CreateIntToPtr(Ptr, Type::getInt64PtrTy(C));
    Ptr = Builder.CreateInBoundsGEP(Ty, IntToPtr, Index);
  }

  // Create store instruction
  Builder.CreateStore(Val, Ptr);

  return true;
}

bool RISCV64MachineInstructionRaiser::raisePCRelativeAccess(
    const MachineInstr &MI, int MBBNo) {
  BasicBlock *BB = getBasicBlock(MBBNo);
  IRBuilder<> Builder(BB);

  const MachineOperand &MOp1 = MI.getOperand(0);
  const MachineOperand &MOp2 = MI.getOperand(1);

  assert(MOp1.getReg() && MOp2.isImm());

  // Find corresponding ADDI or LD instruction
  auto Pred = [&MOp1](const MachineInstr &MI) {
    return (MI.getOpcode() == ADDI || MI.getOpcode() == LD) &&
           MI.getNumOperands() >= 2 && MI.getOperand(1).isReg() &&
           MI.getOperand(1).getReg() == MOp1.getReg();
  };
  auto NextMI = std::find_if(MI.getNextNode()->getIterator(),
                         MI.getParent()->instr_end(), Pred);
  if (NextMI == MI.getParent()->instr_end()) {
    printFailure(MI, "no corresponding ADDI or LD found for AUIPC or LUI");
    return false;
  }

  const MachineOperand &NextMOp1 = NextMI->getOperand(0);
  const MachineOperand &NextMOp3 = NextMI->getOperand(2);

  assert(NextMOp1.isReg() && NextMOp3.isImm());

  // Compute the PC-relative offset
  uint64_t PCOffset = MOp2.getImm() << 12;
  uint64_t InstOffset = MCIR->getMCInstIndex(MI);
  uint64_t TextOffset = MR->getTextSectionAddress();
  int64_t ValueOffset = NextMOp3.getImm();

  uint64_t Offset = 0;
  // PC-relative offset
  if (MI.getOpcode() == AUIPC) {
    Offset = InstOffset + TextOffset + PCOffset + ValueOffset;
  }
  // Absolute offset
  else if (MI.getOpcode() == LUI) {
    Offset = PCOffset + ValueOffset;
  }

  // Try to resolve the offset to a dynamic relocation
  GlobalVariable *GlobalVar = ELFUtils.getDynRelocValueAtOffset(Offset);
  if (GlobalVar != nullptr) {
    ValueTracker.setRegValue(MBBNo, NextMOp1.getReg(), GlobalVar);
    return true;
  }

  // Try to resolve the offset to a .rodata section value
  Value *Index = nullptr;
  GlobalVar = ELFUtils.getRODataValueAtOffset(Offset, Index);
  if (GlobalVar != nullptr) {
    Type *Ty = GlobalVar->getValueType();
    Value *GEP = Builder.CreateInBoundsGEP(Ty, GlobalVar, {Zero, Index});
    ValueTracker.setRegValue(MBBNo, NextMOp1.getReg(), GEP);
    return true;
  }

  // Try to resolve the offset to a .data section value
  GlobalVar = ELFUtils.getDataValueAtOffset(Offset);
  if (GlobalVar != nullptr) {
    ValueTracker.setRegValue(MBBNo, NextMOp1.getReg(), GlobalVar);
    return true;
  }

  printFailure(MI, "Global value not found");
  return false;
}

bool RISCV64MachineInstructionRaiser::raiseCall(const MachineInstr &MI,
                                                int MBBNo) {
  BasicBlock *BB = getBasicBlock(MBBNo);
  IRBuilder<> Builder(BB);

  const MachineOperand &MOp1 = MI.getOperand(0);
  assert(MOp1.isReg());

  if (MOp1.getReg() == X0) {
    printFailure(MI, "zero link register not yet supported");
    return false;
  }

  // Get function at the specified offset
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

      // For vararg function calls, only add arguments that are defined locally
      if (CalledFunctionType->isVarArg() &&
          !isRegisterDefined(ArgReg, MI.getParent()->instr_begin(),
                             MI.getIterator())) {
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

  // Create call instruction and assign to return register
  CallInst *Call = Builder.CreateCall(CalledFunction, Args);
  ValueTracker.setRegValue(MBBNo, X10, Call);

  return true;
}

bool RISCV64MachineInstructionRaiser::raiseReturn(const MachineInstr &MI,
                                                  int MBBNo) {
  BasicBlock *BB = getBasicBlock(MBBNo);
  IRBuilder<> Builder(BB);

  Type *RetTy = BB->getParent()->getReturnType();

  if (RetTy->isVoidTy()) {
    Builder.CreateRetVoid();
  } else {
    // Get return value
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

    // Create return instruction
    Builder.CreateRet(RetVal);
  }

  return true;
}

bool RISCV64MachineInstructionRaiser::raiseUnconditonalBranch(
    ControlTransferInfo *Info) {
  IRBuilder<> Builder(Info->CandidateBlock);

  const MachineInstr &MI = *Info->CandidateMachineInstr;

  const MachineOperand &MOp = MI.getOperand(0);

  assert(MOp.isImm());

  // Get basic block at specified offset
  BasicBlock *Dest = getBasicBlockAtOffset(MI, MOp.getImm());
  if (Dest == nullptr) {
    printFailure(MI, "A BB has not been created for the specified MBBNo");
    return false;
  }

  // Create unconditional branch instruction
  Builder.CreateBr(Dest);

  return true;
}

bool RISCV64MachineInstructionRaiser::raiseConditionalBranch(
    Predicate Pred, ControlTransferInfo *Info) {
  IRBuilder<> Builder(Info->CandidateBlock);

  const MachineInstr &MI = *Info->CandidateMachineInstr;

  const MachineOperand &MOp1 = MI.getOperand(0);
  const MachineOperand &MOp2 = MI.getOperand(1);
  const MachineOperand &MOp3 = MI.getOperand(2);

  // Get righ-hand side value. Offset is either in second or third operand,
  // depending on if the instruction has an implicit zero.
  Value *RHS = nullptr;
  uint64_t Offset;
  if (MI.getOpcode() == C_BNEZ || MI.getOpcode() == C_BEQZ) {
    assert(MOp1.isReg() && MOp2.isImm());
    RHS = Zero;
    Offset = MOp2.getImm();
  } else {
    assert(MOp1.isReg() && MOp2.isReg() && MOp3.isImm());
    RHS = Info->RegValues[1];

    if (RHS == nullptr) {
      printFailure(MI, "RHS of branch instruction is not set");
      return false;
    }

    Offset = MOp3.getImm();
  }

  // Get left-hand side value
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
  int64_t RelativeOffset = MOp2.getImm();
  uint64_t Offset = InstructionOffset + TextSectionOffset + RelativeOffset;

  // Try to resolve the offset to a raised function
  CalledFunction = MR->getRaisedFunctionAt(Offset);

  // Try to resolve the offset to a function using the .plt section
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
