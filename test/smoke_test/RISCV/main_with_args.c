// REQUIRES: system-linux
// RUN: riscv64-linux-gnu-gcc -o %t %s
// RUN: llvm-mctoll -d %t -I /usr/include/stdio.h
// RUN: lli %t-dis.ll a b c | FileCheck %s
// CHECK: 4 arguments:
// CHECK: Argument 1: a
// CHECK: Argument 2: b
// CHECK: Argument 3: c

#include <stdio.h>

int main(int argc, char *argv[]) {
    printf("%d arguments:\n", argc);
    for (int I = 0; I < argc; I++) {
        printf("Argument %d: %s\n", I, argv[I]);
    }
    return 0;
}
