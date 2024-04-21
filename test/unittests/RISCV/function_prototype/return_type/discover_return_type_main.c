// RUN: riscv64-linux-gnu-gcc -o %t %s
// RUN: llvm-mctoll -d -debug %t 2>&1 | FileCheck %s
// CHECK: declare dso_local i32 @main()

#include <stdio.h>

int main(void) {
    printf("Hello, World!\n");
    return 0;
}
