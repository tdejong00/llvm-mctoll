// RUN: riscv64-linux-gnu-gcc -o %t %s
// RUN: llvm-mctoll -d -debug -I /usr/include/stdio.h %t 2>&1 | FileCheck %s
// CHECK: declare dso_local i32 @func()

#include <stdio.h>

int G = 4;

int func(void) {
    printf("%d\n", G);
    G = 5;
    printf("%d\n", G);
    return G;
}

int main(void) {
    return func();
}
