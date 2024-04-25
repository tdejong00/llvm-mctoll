// REQUIRES: system-linux
// RUN: riscv64-linux-gnu-gcc -o %t %s
// RUN: llvm-mctoll -d -debug %t -I /usr/include/stdio.h
// RUN: lli %t-dis.ll | FileCheck %s
// CHECK: 5
// CHECK: A
// CHECK: 7
// CHECK: B

#include <stdio.h>

struct Test {
    int X;
    char C;
};

int main(void) {
    struct Test T = { 5, 'A' };
    printf("%d\n", T.X);
    printf("%c\n", T.C);
    T.X = 7;
    T.C = 'B';
    printf("%d\n", T.X);
    printf("%c\n", T.C);
    return 0;
}
