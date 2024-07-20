// REQUIRES: system-linux
// REQUIRES: riscv64-unknown-linux-gnu-gcc
// RUN: riscv64-unknown-linux-gnu-gcc -o %t %s
// RUN: llvm-mctoll -d -debug %t --include-files=/usr/include/assert.h,/usr/include/stdio.h
// RUN: lli %t-dis.ll | FileCheck %s
// CHECK: passedpassed

#include <assert.h>
#include <stdio.h>

void func(int x) {
    assert(x >= 0);
    printf("passed");
}

int main(void) {
    func(12);
    func(34);
    return 0;
}
