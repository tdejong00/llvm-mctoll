// REQUIRES: system-linux
// REQUIRES: riscv64-linux-gnu-gcc
// RUN: riscv64-linux-gnu-gcc -o %t %s
// RUN: llvm-mctoll -d -debug %t -I /usr/include/stdio.h
// RUN: lli %t-dis.ll | FileCheck %s
// CHECK: factorial(5) = 120
// CHECK: factorial(7) = 5040
// CHECK: factorial(9) = 362880

#include <stdio.h>

unsigned long long factorial(unsigned n) {
    if (n == 0) {
        return 1;
    }
    return n * factorial(n - 1);
}

int main(void) {
    printf("factorial(5) = %llu\n", factorial(5));
    printf("factorial(7) = %llu\n", factorial(7));
    printf("factorial(9) = %llu\n", factorial(9));
    return 0;
}
