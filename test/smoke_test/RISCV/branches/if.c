// REQUIRES: system-linux
// REQUIRES: riscv64-unknown-linux-gnu-gcc
// RUN: riscv64-unknown-linux-gnu-gcc -o %t %s
// RUN: llvm-mctoll -d -debug %t -I /usr/include/stdio.h
// RUN: lli %t-dis.ll | FileCheck %s
// CHECK: yes

#include <stdio.h>

void func(int n) {
    if (n > 5) {
        printf("yes\n");
    } else {
        printf("no\n");
    }
    printf("end\n");
}

int main(void) {
    func(6);
    return 0;
}
