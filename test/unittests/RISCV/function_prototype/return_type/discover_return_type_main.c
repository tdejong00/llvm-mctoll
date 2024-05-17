// REQUIRES: system-linux
// REQUIRES: riscv64-linux-gnu-gcc
// RUN: riscv64-linux-gnu-gcc -fno-stack-protector -o %t %s
// RUN: llvm-mctoll -d -debug -I /usr/include/stdio.h %t 2>&1 | FileCheck %s
// CHECK: declare dso_local i32 @main()

#include <stdio.h>

int main(void) {
    printf("Hello, World!\n");
    return 0;
}
