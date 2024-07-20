// REQUIRES: system-linux
// REQUIRES: riscv64-unknown-linux-gnu-gcc
// RUN: riscv64-unknown-linux-gnu-gcc -fno-stack-protector -o %t %s
// RUN: llvm-mctoll -d -debug %t -I /usr/include/stdio.h
// RUN: lli %t-dis.ll | FileCheck %s
// CHECK: not zero
// CHECK: 17000000000000061234 + 500000000000009876 = 17500000000000071110

#include <stdio.h>

int main(void) {
    unsigned long long int a = 17000000000000061234ULL;
    unsigned long long int b =   500000000000009876ULL;
    unsigned long long int c = a + b;
    if (c != 0) {
        printf("not zero\n");
    }
    printf("%llu + %llu = %llu\n", a, b, c);
}
