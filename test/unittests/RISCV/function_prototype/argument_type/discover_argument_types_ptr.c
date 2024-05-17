// REQUIRES: system-linux
// REQUIRES: riscv64-linux-gnu-gcc
// RUN: riscv64-linux-gnu-gcc -fno-stack-protector -o %t %s
// RUN: llvm-mctoll -d -debug %t 2>&1 | FileCheck %s
// CHECK: declare dso_local i32 @func(i64 %0, i64 %1)

int func(int *a, int *b) {
    return *a + *b;
}

int main() {
    int a = 3, b = 5;
    return func(&a, &b);
}
