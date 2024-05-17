// REQUIRES: system-linux
// REQUIRES: riscv64-linux-gnu-gcc
// RUN: riscv64-linux-gnu-gcc -fno-stack-protector -o %t %s
// RUN: llvm-mctoll -d -debug %t 2>&1 | FileCheck %s
// CHECK: declare dso_local i32 @func(i64 %0, i32 %1, i64 %2)

int func(int *a, int b, int *c) {
    return *a + b + *c;
}

int main(void) {
    int a = 3, b = 5, c = 7;
    return func(&a, b, &c);
}
