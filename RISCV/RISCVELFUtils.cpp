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
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/Object/ObjectFile.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <vector>
#include <zconf.h>

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
                                                 uint64_t Offset,
                                                 uint64_t Length) const {
  if (Section == SectionRef() || !Section.getContents()) {
    return ArrayRef<Byte>();
  }

  StringRef SectionContents =
      unwrapOrError(Section.getContents(), ELFObjectFile->getFileName());
  const unsigned char *Data = SectionContents.bytes_begin() + Offset;

  if (Length == 0) {
    Length = Section.getSize() - Offset;
  }

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

  MCInst Instruction;
  uint64_t InstructionSize;
  uint64_t InstructionOffset = Offset;
  bool Success = false;

  // Disassemble AUIPC instruction
  Success = MR->getMCDisassembler()->getInstruction(
      Instruction, InstructionSize,
      SectionContents.slice(InstructionOffset - Section.getAddress()),
      InstructionOffset, nulls());
  assert(Success && "Failed to disassemble AUIPC instruction in PLT");
  assert(Instruction.getOpcode() == RISCV::AUIPC &&
         "expected AUIPC instruction");

  uint64_t AUIPCOffset = Instruction.getOperand(1).getImm() << 12;

  // Disassemble LD instruction
  Success = MR->getMCDisassembler()->getInstruction(
      Instruction, InstructionSize,
      SectionContents.slice(InstructionOffset + InstructionSize -
                            Section.getAddress()),
      InstructionOffset, nulls());
  assert(Success && "Failed to disassemble LD instruction in PLT");
  assert(Instruction.getOpcode() == RISCV::LD && "expected LD instruction");

  int64_t LDOffset = Instruction.getOperand(2).getImm();

  uint64_t RelocationOffset = InstructionOffset + AUIPCOffset + LDOffset;

  return MR->getDynRelocAtOffset(RelocationOffset);
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
    if (CalledFunction == nullptr) {
      exit(EXIT_FAILURE);
    }
  }

  return CalledFunction;
}

GlobalVariable *RISCVELFUtils::getRODataValueAtOffset(uint64_t Offset,
                                                      Value *&Index) const {
  const std::string SectionName = ".rodata";

  SectionRef Section = getSectionAtOffset(Offset, SectionName);
  if (Section == SectionRef()) {
    return nullptr;
  }

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

  Index = ConstantInt::get(getDefaultIntType(C), Offset - Section.getAddress());

  return GlobalVar;
}

GlobalVariable *RISCVELFUtils::getDataValueAtOffset(uint64_t Offset) const {
  ELFSymbolRef Symbol = getSymbolAtOffset(Offset);
  StringRef SymbolName =
      unwrapOrError(Symbol.getName(), ELFObjectFile->getFileName());

  // Check if global variable already created
  GlobalVariable *GlobalVar = MR->getModule()->getNamedGlobal(SymbolName);
  if (GlobalVar != nullptr) {
    return GlobalVar;
  }

  // Get section contents
  const std::string SectionName = ".data";
  SectionRef Section = getSectionAtOffset(Offset, SectionName);
  ArrayRef<Byte> SectionContents;
  if (Section != SectionRef()) {
    SectionContents = getSectionContents(Section, Offset - Section.getAddress(),
                                         Symbol.getSize());
  }

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
    Align = 4;
    break;
  case 2:
    Ty = Type::getInt16Ty(C);
    Align = 2;
    break;
  case 1:
    Ty = Type::getInt8Ty(C);
    Align = 1;
    break;
  default:
    // More than 4 bytes -> must be array or struct
    IntegerType *ElementType = getDefaultIntType(C);
    Align = ElementType->getBitWidth() / 8;
    uint64_t NumElements = Symbol.getSize() / Align;
    Ty = ArrayType::get(ElementType, NumElements);
  }

  // Determine initial value
  Constant *Initializer = nullptr;
  if (Ty->isIntegerTy()) {
    // Convert the array of bytes to a constant, by combining
    // the bytes into a 32-bit integer in little-endian format
    uint32_t InitVal = 0, Shift = 0;
    for (Byte B : SectionContents) {
      InitVal |= static_cast<uint32_t>(B) << Shift;
      Shift += 8;
    }
    Initializer = ConstantInt::get(Ty, InitVal);
  } else if (Ty->isArrayTy()) {
    // Determine width of element type
    ArrayType *ArrayTy = dyn_cast<ArrayType>(Ty);
    unsigned int ElementWidth = ArrayTy->getElementType()->getIntegerBitWidth();

    // Convert the array of bytes to an array of constants, by combining
    // the bytes into 32-bit integers in little-endian format
    vector<uint32_t> InitVals;
    uint32_t InitVal = 0, Shift = 0;
    for (Byte B : SectionContents) {
      InitVal |= (static_cast<uint32_t>(B) << Shift);
      Shift += 8;
      if (Shift == ElementWidth) {
        InitVals.push_back(InitVal);
        InitVal = 0;
        Shift = 0;
      }
    }

    // If the section was empty, initialize with zeros
    if (InitVals.empty()) {
      InitVals = vector<uint32_t>(Symbol.getSize() / Align);
    }

    Initializer = ConstantDataArray::get(C, InitVals);
  }

  // Create global variable
  GlobalVar = new GlobalVariable(*MR->getModule(), Ty, false, Linkage,
                                 Initializer, SymbolName);
  GlobalVar->setAlignment(MaybeAlign(Align));
  GlobalVar->setDSOLocal(true);

  return GlobalVar;
}

GlobalVariable *RISCVELFUtils::getDynRelocValueAtOffset(uint64_t Offset) const {
  const RelocationRef *DynReloc = MR->getDynRelocAtOffset(Offset);
  if (DynReloc == nullptr) {
    return nullptr;
  }

  dbgs() << "found dyn reloc at offset " << DynReloc->getOffset() << "\n";

  StringRef SymbolName = unwrapOrError(DynReloc->getSymbol()->getName(),
                                       ELFObjectFile->getFileName());

  dbgs() << "with name " << SymbolName << "\n";

  // Check if global variable already created
  GlobalVariable *GlobalVar = MR->getModule()->getNamedGlobal(SymbolName);
  if (GlobalVar != nullptr) {
    return GlobalVar;
  }

  // Get symbol of dyn reloc
  const auto *Symbol = unwrapOrError(
      ELFObjectFile->getSymbol(DynReloc->getSymbol()->getRawDataRefImpl()),
      ELFObjectFile->getFileName());
  uint64_t SymbolSize = Symbol->st_size;

  dbgs() << "with size " << SymbolSize << "\n";

  // Get section contents
  SectionRef Section = getSectionAtOffset(Offset);
  ArrayRef<Byte> SectionContents;
  if (Section != SectionRef()) {
    SectionContents =
        getSectionContents(Section, Offset - Section.getAddress(), SymbolSize);
  }

  // Determine linkage type
  GlobalValue::LinkageTypes Linkage;
  switch (Symbol->getBinding()) {
  case ELF::STT_COMMON:
    Linkage = GlobalValue::CommonLinkage;
    break;
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
  switch (SymbolSize) {
  case 4:
    Ty = Type::getInt32Ty(C);
    Align = 4;
    break;
  case 2:
    Ty = Type::getInt16Ty(C);
    Align = 2;
    break;
  case 1:
    Ty = Type::getInt8Ty(C);
    Align = 1;
    break;
  case 0:
    Ty = getDefaultPtrType(C);
    Linkage = GlobalValue::ExternalLinkage;
    Align = 8;
    break;
  default:
    // More than 4 bytes -> must be array or struct
    IntegerType *ElementType = getDefaultIntType(C);
    Align = ElementType->getBitWidth() / 8;
    uint64_t NumElements = SymbolSize / Align;
    Ty = ArrayType::get(ElementType, NumElements);
  }

  dbgs() << "with type ";
  Ty->dump();

  // Create global variable
  GlobalVar = new GlobalVariable(*MR->getModule(), Ty, false, Linkage, nullptr,
                                 SymbolName);
  GlobalVar->setAlignment(MaybeAlign(Align));

  return GlobalVar;
}
