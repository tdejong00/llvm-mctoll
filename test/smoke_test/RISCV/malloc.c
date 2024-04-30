// REQUIRES: system-linux
// RUN: riscv64-linux-gnu-gcc -fno-stack-protector -o %t %s
// RUN: llvm-mctoll -d -debug %t --include-files=/usr/include/stdio.h,/usr/include/stdlib.h
// RUN: lli %t-dis.ll | FileCheck %s
// CHECK: 5

#include <stdio.h>
#include <stdlib.h>

int main(void) {
    int *X = (int *)malloc(sizeof(int));

    *X = 5;

    printf("%d\n", *X);

    free(X);

    return 0;
}