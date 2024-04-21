// REQUIRES: system-linux
// RUN: riscv64-linux-gnu-gcc -o %t %s
// RUN: llvm-mctoll -d %t -I /usr/include/stdio.h
// RUN: lli %t-dis.ll | FileCheck %s
// CHECK: 5
// CHECK: A

#include <stdio.h>

struct Test {
    int X;
    char C;
} Test;

int main(void) {
    struct Test T = { 5, 'A' };
    printf("%d\n", T.X);
    printf("%c\n", T.C);
    return 0;
}
