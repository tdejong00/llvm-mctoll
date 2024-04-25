// REQUIRES: system-linux
// RUN: riscv64-linux-gnu-gcc -o %t %s
// RUN: llvm-mctoll -d -debug %t -I /usr/include/stdio.h
// RUN: lli %t-dis.ll | FileCheck %s
// CHECK: factorial(5) = 120

#include <stdio.h>

int factorial(int N) {
  if (N == 0) {
    return 1;
  }
  return N * factorial(N - 1);
}

int main(void) {
    int F = factorial(5);
    printf("factorial(5) = %d\n", F);
    return 0;
}
