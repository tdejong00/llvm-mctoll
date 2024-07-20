// NOTE: failing because global symbols with read-only data not implemented yet

// UNSUPPORTED: not-implemented
// REQUIRES: system-linux
// REQUIRES: riscv64-unknown-linux-gnu-gcc
// RUN: riscv64-unknown-linux-gnu-gcc -o %t %s
// RUN: llvm-mctoll -d -debug %t -I /usr/include/stdio.h
// RUN: lli %t-dis.ll | FileCheck %s
// CHECK: Hello, World!

#include <stdio.h>

const char *C = "Hello, World!";

int main(void) {
    puts(C);
    return 0;
}
