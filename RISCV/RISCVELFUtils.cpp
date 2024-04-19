//===-- RISCVELFUtils.cpp ---------------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains the definition of multiple utility functions regarding
// ELF sections and symbols for use by llvm-mctoll.
//
//===----------------------------------------------------------------------===//

#include "RISCVELFUtils.h"
#include "IncludedFileInfo.h"
#include "MCTargetDesc/RISCVMCTargetDesc.h"
#include "ModuleRaiser.h"
#include "RISCV64/RISCV64MachineInstructionUtils.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/Object/ObjectFile.h"
#include "llvm/TableGen/Record.h"
#include <cassert>
#include <cstdint>

using namespace llvm;
using namespace mctoll;
using namespace RISCV64MachineInstructionUtils;

SectionRef RISCVELFUtils::getSectionAtOffset(uint64_t Offset,
                                             StringRef Name) const {
  for (SectionRef Section : ELFObjectFile->sections()) {
    uint64_t SectionAddress = Section.getAddress();
    uint64_t SectionSize = Section.getSize();

    // Find section which contains the given offset
    if (SectionAddress <= Offset && SectionAddress + SectionSize >= Offset) {
      StringRef SectionName =
          unwrapOrError(Section.getName(), ELFObjectFile->getFileName());

      // Check if section has expected name
      if (Name.empty() || !SectionName.equals(Name)) {
        return SectionRef();
      }

      return Section;
    }
  }

  return SectionRef();
}

ArrayRef<Byte> RISCVELFUtils::getSectionContents(SectionRef Section,
                                                 uint64_t Offset) const {
  if (Section == SectionRef() || !Section.getContents()) {
    return ArrayRef<Byte>();
  }

  StringRef SectionContents =
      unwrapOrError(Section.getContents(), ELFObjectFile->getFileName());
  const unsigned char *Data = SectionContents.bytes_begin() + Offset;
  uint64_t Length = Section.getSize() - Offset;

  return makeArrayRef(Data, Length);
}

ELFSymbolRef RISCVELFUtils::getSymbolAtOffset(uint64_t Offset) const {
  for (const ELFSymbolRef &Symbol : ELFObjectFile->symbols()) {
    uint64_t SymbolAddress =
        unwrapOrError(Symbol.getAddress(), ELFObjectFile->getFileName());
    uint64_t SymbolSize = Symbol.getSize();

    // Find symbol which contains the given offset
    if (SymbolAddress <= Offset && SymbolAddress + SymbolSize >= Offset) {
      return Symbol;
    }
  }

  return SymbolRef();
}

const RelocationRef *
RISCVELFUtils::getRelocationAtOffset(uint64_t Offset) const {
  const std::string SectionName = ".plt";
  SectionRef Section = getSectionAtOffset(Offset, SectionName);
  ArrayRef<Byte> SectionContents = getSectionContents(Section);

  if (SectionContents.empty()) {
    return nullptr;
  }

  // Disassemble instruction
  MCInst Instruction;
  uint64_t InstructionSize;
  uint64_t InstructionOffset = Offset;
  bool Success = MR->getMCDisassembler()->getInstruction(
      Instruction, InstructionSize,
      SectionContents.slice(InstructionOffset - Section.getAddress()),
      InstructionOffset, nulls());
  assert(Success && "Failed to disassemble instruction in PLT");

  // If first instruction is AUIPC, skip to the next instruction
  if (Instruction.getOpcode() == RISCV::AUIPC) {
    Success = MR->getMCDisassembler()->getInstruction(
        Instruction, InstructionSize,
        SectionContents.slice(InstructionOffset + InstructionSize -
                              Section.getAddress()),
        InstructionOffset, nulls());
    assert(Success && "Failed to disassemble instruction in PLT");
  }

  const MCOperand &MOp1 = Instruction.getOperand(0);
  const MCOperand &MOp2 = Instruction.getOperand(1);
  const MCOperand &MOp3 = Instruction.getOperand(2);

  assert(MOp1.isReg() && MOp2.isReg() && MOp3.isImm());

  int64_t PCOffset = MOp3.getImm();

  // Calculate offset of relocated function
  // TODO: hardcoded offset 0x2000 is probably not correct
  uint64_t RelocationOffset = InstructionOffset + PCOffset + 0x2000;

  const RelocationRef *Relocation = MR->getDynRelocAtOffset(RelocationOffset);

  return Relocation;
}

Function *RISCVELFUtils::getFunctionAtOffset(uint64_t Offset) const {
  const RelocationRef *Relocation = getRelocationAtOffset(Offset);

  // Check if relocation was found
  if (Relocation == nullptr) {
    return nullptr;
  }

  symbol_iterator Symbol = Relocation->getSymbol();
  assert(Symbol != ELFObjectFile->symbol_end() &&
         "Failed to find relocation symbol for PLT entry");
  StringRef SymbolName =
      unwrapOrError(Symbol->getName(), ELFObjectFile->getFileName());
  uint64_t SymbolAddress =
      unwrapOrError(Symbol->getAddress(), ELFObjectFile->getFileName());

  // Attempt to get function using symbol address
  Function *CalledFunction = MR->getRaisedFunctionAt(SymbolAddress);

  // As a last resort, use information from manually included files
  if (CalledFunction == nullptr) {
    CalledFunction = IncludedFileInfo::CreateFunction(
        SymbolName, *const_cast<ModuleRaiser *>(MR));
  }

  return CalledFunction;
}

GlobalVariable *
RISCVELFUtils::getRODataValueAtOffset(uint64_t Offset,
                                      Value *&UpperBound) const {
  const std::string SectionName = ".rodata";

  SectionRef Section = getSectionAtOffset(Offset, SectionName);
  ArrayRef<Byte> SectionContents = getSectionContents(Section);

  if (SectionContents.empty()) {
    return nullptr;
  }

  std::string ValueName = SectionName + std::to_string(Section.getIndex());

  // Check if global variable already created, create it if not
  GlobalVariable *GlobalVar = MR->getModule()->getNamedGlobal(ValueName);
  if (GlobalVar == nullptr) {
    Constant *DataConstant = ConstantDataArray::get(C, SectionContents);

    GlobalVar = new GlobalVariable(*MR->getModule(), DataConstant->getType(),
                                   true, GlobalValue::PrivateLinkage,
                                   DataConstant, ValueName);
    GlobalVar->setAlignment(MaybeAlign(Section.getAlignment()));
    GlobalVar->setUnnamedAddr(GlobalValue::UnnamedAddr::Global);
  }

  UpperBound =
      ConstantInt::get(Type::getInt32Ty(C), Offset - Section.getAddress());

  return GlobalVar;
}

GlobalVariable *RISCVELFUtils::getDataValueAtOffset(uint64_t Offset) const {
  ELFSymbolRef Symbol = getSymbolAtOffset(Offset);
  StringRef SymbolName =
      unwrapOrError(Symbol.getName(), ELFObjectFile->getFileName());

  const std::string SectionName = ".data";
  SectionRef Section = getSectionAtOffset(Offset, SectionName);
  ArrayRef<Byte> SectionContents =
      getSectionContents(Section, Offset - Section.getAddress());

  assert(!SectionContents.empty() && "section data is unexpectedly empty");

  // Check if global variable already created, create it if not
  GlobalVariable *GlobalVar = MR->getModule()->getNamedGlobal(SymbolName);
  if (GlobalVar == nullptr) {
    // Determine linkage type
    GlobalValue::LinkageTypes Linkage;
    switch (Symbol.getBinding()) {
    case ELF::STB_GLOBAL:
      Linkage = GlobalValue::ExternalLinkage;
      break;
    case ELF::STB_LOCAL:
      Linkage = GlobalValue::InternalLinkage;
      break;
    default:
      Linkage = GlobalValue::ExternalLinkage;
      break;
    }

    // Determine size and alignment
    Type *Ty;
    uint64_t Align = 0;
    switch (Symbol.getSize()) {
    case 4:
      Ty = Type::getInt32Ty(C);
      Align = 32;
      break;
    case 2:
      Ty = Type::getInt16Ty(C);
      Align = 16;
      break;
    case 1:
      Ty = Type::getInt8Ty(C);
      Align = 8;
      break;
    default:
      Align = 8;
      Ty = ArrayType::get(Type::getInt8Ty(C), SectionContents.size());
    }

    // Determine initial value
    Constant *Initializer = nullptr;
    if (Ty->isIntegerTy()) {
      // Convert the array of bytes to a constant
      uint64_t InitVal = 0, Shift = 0;
      for (Byte B : SectionContents) {
        InitVal = (B << Shift) | InitVal;
        Shift += 8;
      }
      Initializer = ConstantInt::get(Ty, InitVal);
    } else if (Ty->isArrayTy()) {
      Initializer = ConstantDataArray::get(C, SectionContents);
    }

    // Create global variable
    GlobalVar = new GlobalVariable(*MR->getModule(), Ty, false, Linkage,
                                   Initializer, SymbolName);
    GlobalVar->setAlignment(MaybeAlign(Align));
    GlobalVar->setDSOLocal(true);
  }

  return GlobalVar;
}
