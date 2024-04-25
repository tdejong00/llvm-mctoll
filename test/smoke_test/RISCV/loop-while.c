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
    int I = 0;
    while (I < 5) {
        printf("%d\n", I);
        I++;
    }
    return 0;
}
