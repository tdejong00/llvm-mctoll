// REQUIRES: system-linux
// REQUIRES: riscv64-linux-gnu-gcc
// RUN: riscv64-linux-gnu-gcc -o %t %s
// RUN: llvm-mctoll -d -debug %t --include-files=/usr/include/stdio.h,/usr/include/string.h
// RUN: lli %t-dis.ll | FileCheck %s
// CHECK: Hello == World: -15
// CHECK: Hello == Hello: 0

#include <stdio.h>
#include <string.h>

int mystrcmp(const void *s1, const void *s2) {
  return strcmp((const char *)s1, (const char *)s2);
}

int main(void) {
    int y1 = mystrcmp("Hello", "World");
    int y2 = mystrcmp("Hello", "Hello");
    printf("Hello == World: %d\n", y1);
    printf("Hello == Hello: %d\n", y2);
    return 0;
}
