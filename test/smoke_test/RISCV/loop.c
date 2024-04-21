// REQUIRES: system-linux
// RUN: riscv64-linux-gnu-gcc -fno-stack-protector -o %t %s
// RUN: llvm-mctoll -d %t -I /usr/include/stdio.h
// RUN: lli %t-dis.ll | FileCheck %s
// CHECK: 0
// CHECK: 1
// CHECK: 2
// CHECK: 3
// CHECK: 4

#include <stdio.h>

int main(void) {
    for (int I = 0; I < 5; I++) {
        printf("%d\n", I);
    }
    return 0;
}
