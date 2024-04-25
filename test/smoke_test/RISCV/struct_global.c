// REQUIRES: system-linux
// RUN: riscv64-linux-gnu-gcc -o %t %s
// RUN: llvm-mctoll -d %t -I /usr/include/stdio.h
// RUN: lli %t-dis.ll | FileCheck %s
// CHECK: 5
// CHECK: A
// CHECK: 7
// CHECK: B

#include <stdio.h>

struct Test {
    int X;
    char C;
} T = { 5, 'A' };

int main(void) {
    printf("%d\n", T.X);
    printf("%c\n", T.C);
    T.X = 7;
    T.C = 'B';
    printf("%d\n", T.X);
    printf("%c\n", T.C);
    return 0;
}
