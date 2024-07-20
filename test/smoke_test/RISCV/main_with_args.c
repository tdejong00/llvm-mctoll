// REQUIRES: system-linux
// REQUIRES: riscv64-unknown-linux-gnu-gcc
// RUN: riscv64-unknown-linux-gnu-gcc -o %t %s
// RUN: llvm-mctoll -d -debug %t -I /usr/include/stdio.h
// RUN: lli %t-dis.ll a b c | FileCheck %s
// CHECK: 4 arguments:
// CHECK: Argument 1: a
// CHECK: Argument 2: b
// CHECK: Argument 3: c

#include <stdio.h>

int main(int argc, char *argv[]) {
    printf("%d arguments:\n", argc);
    for (int i = 0; i < argc; i++) {
        printf("Argument %d: %s\n", i, argv[i]);
    }
    return 0;
}
