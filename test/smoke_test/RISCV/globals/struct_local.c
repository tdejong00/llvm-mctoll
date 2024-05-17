// REQUIRES: system-linux
// REQUIRES: riscv64-linux-gnu-gcc
// RUN: riscv64-linux-gnu-gcc -o %t %s
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
};

int main(void) {
    struct Test t = { 5, 'A' };
    printf("%d\n", t.x);
    printf("%c\n", t.c);
    t.x = 7;
    t.c = 'B';
    printf("%d\n", t.x);
    printf("%c\n", t.c);
    return 0;
}
