// REQUIRES: system-linux
// RUN: riscv64-linux-gnu-gcc -fno-stack-protector -o %t %s
// RUN: llvm-mctoll -d -debug %t -I /usr/include/stdio.h
// RUN: lli %t-dis.ll | FileCheck %s
// CHECK: 3
// CHECK: 7
// CHECK: 11
// CHECK: 15
// XFAIL: *

#include <stdio.h>

#define N 4

int main(void) {
    int A[N] = {2,4,6,8};
    int B[N] = {1,3,5, 7};
    for (int I = 0; I < N; I ++) { 
        printf("%d\n", A[I] + B[I]);
    }
    return 0;
}
