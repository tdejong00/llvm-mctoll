// REQUIRES: system-linux
// REQUIRES: riscv64-linux-gnu-gcc
// RUN: riscv64-linux-gnu-gcc -o %t %s
// RUN: llvm-mctoll -d -debug %t 2>&1 | FileCheck %s
// CHECK: declare dso_local i32 @func(i32 %0, i32 %1)

int func(int a, int b) {
    return a + b;
}

int main() {
    return func(3, 5);
}
