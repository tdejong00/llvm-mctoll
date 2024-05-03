// REQUIRES: system-linux
// RUN: riscv64-linux-gnu-gcc -fno-stack-protector -o %t %s
// RUN: llvm-mctoll -d -debug %t -I /usr/include/stdio.h
// RUN: lli %t-dis.ll | FileCheck %s
// CHECK: not zero
// CHECK: 17000000000000061234 + 500000000000009876 = 17500000000000071110

#include <stdio.h>

int main(void) {
    unsigned long long int A = 17000000000000061234ULL;
    unsigned long long int B =   500000000000009876ULL;
    unsigned long long int C = A + B;
    if (C != 0) {
        printf("not zero\n");
    }
    printf("%llu + %llu = %llu\n", A, B, C);
}
