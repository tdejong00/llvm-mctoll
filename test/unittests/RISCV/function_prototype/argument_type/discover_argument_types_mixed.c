// RUN: riscv64-linux-gnu-gcc -o %t %s
// RUN: llvm-mctoll -d -debug %t 2>&1 | FileCheck %s
// CHECK: declare dso_local i32 @func(ptr %0, i32 %1, ptr %2)

int func(int *A, int B, int *C) {
    return *A + B + *C;
}

int main(void) {
    int A = 3, B = 5, C = 7;
    return func(&A, B, &C);
}
