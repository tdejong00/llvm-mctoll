// REQUIRES: system-linux
// RUN: riscv64-linux-gnu-gcc -o %t %s
// RUN: llvm-mctoll -d -debug %t --include-files=/usr/include/assert.h,/usr/include/stdio.h
// RUN: lli %t-dis.ll | FileCheck %s
// CHECK: passed

#include <assert.h>
#include <stdio.h>

void func(int X) {
    assert(X >= 0);
    printf("passed");
}

int main(void) {
    func(12);
    func(34);
    return 0;
}
