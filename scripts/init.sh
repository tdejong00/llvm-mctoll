#!/bin/bash
( [ -z $1 ] || [ -z $2 ] || [ -z $3 ] ) && echo "USAGE: $0 <source-dir> <build-dir> <build-type>" && exit 1
cmake -S $1 -B $2 -G "Ninja" \
  -DLLVM_TARGETS_TO_BUILD="X86;ARM;RISCV" \
  -DLLVM_ENABLE_PROJECTS="clang;lld" \
  -DLLVM_ENABLE_ASSERTIONS=true \
  -DCLANG_DEFAULT_PIE_ON_LINUX=OFF \
  -DCMAKE_BUILD_TYPE=$3 \
  -DLLVM_ENABLE_DUMP=true \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
