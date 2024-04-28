//===-- RISCVELFUtils.h -----------------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains the declaration of multiple utility functions regarding
// ELF sections and symbols for use by llvm-mctoll.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_TOOLS_LLVM_MCTOLL_RISCV_RISCVELFUTILS_H
#define LLVM_TOOLS_LLVM_MCTOLL_RISCV_RISCVELFUTILS_H

#include "ModuleRaiser.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Object/ELFObjectFile.h"
#include "llvm/Object/ObjectFile.h"
#include "llvm/Support/Casting.h"
#include <cstdint>
#include <zconf.h>

using namespace llvm;
using namespace object;

namespace llvm {
namespace mctoll {

/// Provides various functions to extract information from the ELF file, such
/// as sections, symbols, relocations, and values located at specific offsets
/// within sections.
class RISCVELFUtils {
public:
  RISCVELFUtils(const ModuleRaiser *MR, LLVMContext &C)
      : MR(MR), C(C),
        ELFObjectFile(dyn_cast<ELF64LEObjectFile>(MR->getObjectFile())) {}

  /// Gets the section of the ELF which contains the given offset and optionally
  /// checks if it has the given name. If the section could not be found or the
  /// given name does not correspond to the name of the section, a default
  /// constructed section will be returned.
  SectionRef getSectionAtOffset(uint64_t Offset, StringRef Name = "") const;

  /// Extracts the contents of the given ELF section and returns it as an array
  /// of bytes (unsigned chars) using an optional offset to offset the start
  /// of the contents. Returns an empty array if the section is an empty
  /// SectionRef or if the contents of the section are empty.
  ArrayRef<Byte> getSectionContents(SectionRef Section, uint64_t Offset = 0,
                                    uint64_t Length = 0) const;

  /// Gets the ELF symbol which contains the given offset. If the symbol can
  /// not be found, a default constructed symbol will be returned.
  ELFSymbolRef getSymbolAtOffset(uint64_t Offset) const;

  /// Gets the relocation located in the .plt ELF section at the given offset.
  /// Returns null if the section could not be found or if the section is empty.
  const RelocationRef *getRelocationAtOffset(uint64_t Offset) const;

  /// Gets the function using the relocation located in the .plt ELF section
  /// at the given offset. Returns null if the relocation could not be found.
  Function *getFunctionAtOffset(uint64_t Offset) const;

  /// Gets the value located in the .rodata ELF section at the given offset
  /// and creates a corresponding global variable. The given upper bound will
  /// be set to start of the desired value inside the global variable, which is
  /// calculated as the given offset minus the address of the found section.
  /// Returns null when the section could not be found or the contens of the
  /// section are empty.
  GlobalVariable *getRODataValueAtOffset(uint64_t Offset,
                                         Value *&UpperBound) const;

  /// Gets the value located in the .data ELF section at the given offset
  /// and creates a corresponding global variable.
  GlobalVariable *getDataValueAtOffset(uint64_t Offset) const;

private:
  const ModuleRaiser *MR;
  LLVMContext &C;
  const ELF64LEObjectFile *ELFObjectFile;
};

} // namespace mctoll
} // namespace llvm

#endif // LLVM_TOOLS_LLVM_MCTOLL_RISCV_RISCVELFUTILS_H