// NOTE: failing because arrays and structs on stack is not supported yet,
//       using a malloced pointer does work however

// UNSUPPORTED: not-implemented
// REQUIRES: system-linux
// REQUIRES: riscv64-linux-gnu-gcc
// RUN: riscv64-linux-gnu-gcc -fno-stack-protector -o %t %s
// RUN: llvm-mctoll -d -debug %t --include-files=/usr/include/stdio.h,/usr/include/stdlib.h,/usr/include/x86_64-linux-gnu/sys/time.h
// RUN: lli %t-dis.ll | FileCheck %s
// CHECK-NOT: Seconds: 0
// CHECK-NOT: Microseconds: 0

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

int main() {
    struct timeval t;
    gettimeofday(&t, NULL);
    
    printf("Seconds: %ld\n", t.tv_sec);
    printf("Microseconds: %ld\n", t.tv_usec);
    
    return 0;
}
