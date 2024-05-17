// REQUIRES: system-linux
// REQUIRES: riscv64-linux-gnu-gcc
// RUN: riscv64-linux-gnu-gcc -fno-stack-protector -o %t %s
// RUN: llvm-mctoll -d -debug %t --include-files=/usr/include/stdio.h,/usr/include/stdlib.h
// RUN: lli %t-dis.ll | FileCheck %s
// CHECK: 5

#include <stdio.h>
#include <stdlib.h>

int main(void) {
    int *x = (int *)malloc(sizeof(int));

    *x = 5;

    printf("%d\n", *x);

    free(x);

    return 0;
}