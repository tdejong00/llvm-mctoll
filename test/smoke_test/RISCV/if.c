// REQUIRES: system-linux
// RUN: riscv64-linux-gnu-gcc -o %t %s
// RUN: llvm-mctoll -d -debug %t -I /usr/include/stdio.h
// RUN: lli %t-dis.ll | FileCheck %s
// CHECK: yes

#include <stdio.h>

void func(int N) {
    if (N > 5) {
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
