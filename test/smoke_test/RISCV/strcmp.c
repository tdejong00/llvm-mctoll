// REQUIRES: system-linux
// RUN: riscv64-linux-gnu-gcc -o %t %s
// RUN: llvm-mctoll -d %t --include-files=/usr/include/stdio.h,/usr/include/string.h
// RUN: lli %t-dis.ll | FileCheck %s
// CHECK: Hello == World: -15
// CHECK: Hello == Hello: 0

#include <stdio.h>
#include <string.h>

int mystrcmp(const void *S1, const void *S2) {
  return strcmp((const char *)S1, (const char *)S2);
}

int main(void) {
    int Y1 = mystrcmp("Hello", "World");
    int Y2 = mystrcmp("Hello", "Hello");
    printf("Hello == World: %d\n", Y1);
    printf("Hello == Hello: %d\n", Y2);
    return 0;
}
