#!/bin/bash
[ -n "$RISCV_SYSROOT_PATH" ] || {
  echo >&2 "$0: error: environment variable RISCV_SYSROOT_PATH not defined"
  exit 1
}
echo "sysroot: $RISCV_SYSROOT_PATH"

[ -n "$RISCV_TOOLCHAIN_PATH" ] || {
  echo >&2 "$0: error: environment variable RISCV_TOOLCHAIN_PATH not defined"
  exit 1
}
echo "gcc-toolchain: $RISCV_TOOLCHAIN_PATH"

# Compile without optimizations
build/bin/clang -O0 --target=riscv64-linux-gnu -march=rv64gc --sysroot=$RISCV_SYSROOT_PATH --gcc-toolchain=$RISCV_TOOLCHAIN_PATH $1-dis.ll -o $1-raised
echo "recompiled without optimizations: $1-raised"

# Compile with optimizations to .ll, then again with optimizations to exec
build/bin/clang -O3 -S -emit-llvm --target=riscv64-linux-gnu -march=rv64gc --sysroot=$RISCV_SYSROOT_PATH --gcc-toolchain=$RISCV_TOOLCHAIN_PATH $1-dis.ll -o $1-raised-opt.ll
build/bin/clang -O3 --target=riscv64-linux-gnu -march=rv64gc --sysroot=$RISCV_SYSROOT_PATH --gcc-toolchain=$RISCV_TOOLCHAIN_PATH $1-raised-opt.ll -o $1-raised-opt
echo "recompiled with optimizations: $1-raised-opt"
