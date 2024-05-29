// REQUIRES: system-linux
// REQUIRES: riscv64-linux-gnu-gcc
// RUN: riscv64-linux-gnu-gcc -o %t %s
// RUN: llvm-mctoll -d -debug %t -I /usr/include/stdio.h
// RUN: lli %t-dis.ll | FileCheck %s
// CHECK: Hello, World!
// XFAIL: *

#include <stdio.h>

const char *C = "Hello, World!";

int main(void) {
    puts(C);
    return 0;
}
