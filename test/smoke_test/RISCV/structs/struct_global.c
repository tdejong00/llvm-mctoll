// REQUIRES: system-linux
// REQUIRES: riscv64-unknown-linux-gnu-gcc
// RUN: riscv64-unknown-linux-gnu-gcc -o %t %s
// RUN: llvm-mctoll -d -debug %t -I /usr/include/stdio.h
// RUN: lli %t-dis.ll | FileCheck %s
// CHECK: 5
// CHECK: A
// CHECK: 7
// CHECK: B

#include <stdio.h>

struct Test {
    int x;
    char c;
} T = { 5, 'A' };

int main(void) {
    printf("%d\n", T.x);
    printf("%c\n", T.c);
    T.x = 7;
    T.c = 'B';
    printf("%d\n", T.x);
    printf("%c\n", T.c);
    return 0;
}
