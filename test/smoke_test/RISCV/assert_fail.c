// REQUIRES: system-linux
// RUN: riscv64-linux-gnu-gcc -o %t %s
// RUN: llvm-mctoll -d -debug %t -I /usr/include/assert.h
// RUN: lli %t-dis.ll | FileCheck %s
// CHECK: failed
// XFAIL: *

#include <assert.h>

void func(int X) {
    assert(X >= 0 && "failed");
}

int main(void) {
    func(12);
    func(-34);
    return 0;
}
