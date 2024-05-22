// REQUIRES: system-linux
// REQUIRES: riscv64-linux-gnu-gcc
// RUN: riscv64-linux-gnu-gcc -o %t %s
// RUN: llvm-mctoll -d -debug %t -I /usr/include/stdio.h
// RUN: lli %t-dis.ll | FileCheck %s
// CHECK: Hi from func1 with data 'some data'
// CHECK: Hi from func2 with data 'some data'

#include <stdio.h>

void func2(const char *s);

void func1(const char *s) {
    printf("Hi from func1 with data '%s'\n", s);
    func2(s);
}

void func2(const char *s) {
    printf("Hi from func2 with data '%s'\n", s);
}

int main(void) {
    func1("some data");
    return 0;
}
