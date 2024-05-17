// REQUIRES: system-linux
// REQUIRES: riscv64-linux-gnu-gcc
// RUN: riscv64-linux-gnu-gcc -o %t %s
// RUN: llvm-mctoll -d -debug %t -I /usr/include/stdio.h
// RUN: lli %t-dis.ll | FileCheck %s
// CHECK: The quick brown
// CHECK: fox jumps over
// CHECK: the lazy dog
// CHEKC: error

#include <stdio.h>

typedef enum { A, B, C } E;

void func(E x) {
    switch (x) {
        case A:
            printf("The quick brown\n");
            break;
        case B:
            printf("fox jumps over\n");
            break;
        case C:
            printf("the lazy dog\n");
            break;
        default:
            printf("error\n");
            break;
    }
}

int main(void) {
    func(A);
    func(B);
    func(C);
    func((E)32);
    return 0;
}
