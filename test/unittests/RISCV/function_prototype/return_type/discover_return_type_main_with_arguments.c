// REQUIRES: system-linux
// REQUIRES: riscv64-linux-gnu-gcc
// RUN: riscv64-linux-gnu-gcc -o %t %s
// RUN: llvm-mctoll -d -debug -I /usr/include/stdio.h %t 2>&1 | FileCheck %s
// CHECK: declare dso_local i32 @main(i32 %0, i64 %1)

#include <stdio.h>

int main(int argc, char *argv[]) {
    printf("Hello, World!\n");
    return 0;
}
