// REQUIRES: system-linux
// RUN: riscv64-linux-gnu-gcc -fno-stack-protector -o %t %s
// RUN: llvm-mctoll -d %t -I /usr/include/stdio.h
// RUN: lli %t-dis.ll | FileCheck %s
// CHECK: 3
// CHECK: 7
// CHECK: 11
// CHECK: 15

#include <stdio.h>

#define N 4

int A[N] = {2,4,6,8};
int B[N] = {1,3,5, 7};

int main(void) {
    for (int I = 0; I < N; I ++) { 
        printf("%d\n", A[I] + B[I]);
    }
    return 0;
}
