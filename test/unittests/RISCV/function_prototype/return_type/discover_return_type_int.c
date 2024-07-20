// REQUIRES: system-linux
// REQUIRES: riscv64-unknown-linux-gnu-gcc
// RUN: riscv64-unknown-linux-gnu-gcc -o %t %s
// RUN: llvm-mctoll -d -debug %t 2>&1 | FileCheck %s
// CHECK: declare dso_local i32 @func()

int func(void) {
    return 3;
}

int main(void) {
    return func();
}
