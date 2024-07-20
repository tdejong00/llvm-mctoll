// REQUIRES: system-linux
// REQUIRES: riscv64-unknown-linux-gnu-gcc
// RUN: riscv64-unknown-linux-gnu-gcc -o %t %s
// RUN: llvm-mctoll -d -debug %t -I /usr/include/stdio.h
// RUN: lli %t-dis.ll | FileCheck %s
// CHECK: 8
// CHECK: 26

#include <stdio.h>

int C = 8;

int main(void) {
  printf("%d\n", C);
  C = 26;
  printf("%d\n", C);
}
