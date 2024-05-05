// REQUIRES: system-linux
// RUN: riscv64-linux-gnu-gcc -o %t %s
// RUN: llvm-mctoll -d -debug %t -I /usr/include/stdio.h
// RUN: lli %t-dis.ll | FileCheck %s
// CHECK: fibonacci(0) = 0
// CHECK: fibonacci(1) = 1
// CHECK: fibonacci(8) = 21
// CHECK: fibonacci(13) = 233
// CHECK: fibonacci(19) = 4181

#include <stdio.h>

unsigned long long fibonacci(unsigned N) {
    if (N < 2) {
        return N;
    }
    return fibonacci(N - 1) + fibonacci(N - 2);
}

int main(void) {
    printf("fibonacci(0) = %llu\n", fibonacci(0));
    printf("fibonacci(1) = %llu\n", fibonacci(1));
    printf("fibonacci(8) = %llu\n", fibonacci(8));
    printf("fibonacci(13) = %llu\n", fibonacci(13));
    printf("fibonacci(19) = %llu\n", fibonacci(19));
    return 0;
}
