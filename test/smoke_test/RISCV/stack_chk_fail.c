// NOTE: failing because the function __stack_chk_fail cannot
//       be found without the -fno-stack-protector compiler flag


// UNSUPPORTED: not-implemented
// REQUIRES: system-linux
// REQUIRES: riscv64-unknown-linux-gnu-gcc
// RUN: riscv64-unknown-linux-gnu-gcc -o %t %s
// RUN: llvm-mctoll -d -debug %t -I /usr/include/x86_64-linux-gnu/sys/time.h
// RUN: lli %t-dis.ll | FileCheck %s

#include <stdio.h>
#include <sys/time.h>

int main() {
    struct timeval t;
    gettimeofday(&t, NULL);
    return 0;
}
