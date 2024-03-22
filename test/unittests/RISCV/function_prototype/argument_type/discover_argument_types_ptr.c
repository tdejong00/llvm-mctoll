// RUN: riscv64-linux-gnu-gcc -o %t %s
// RUN: llvm-mctoll -d -debug %t 2>&1 | FileCheck %s
// CHECK: declare dso_local i32 @func(ptr %0, ptr %1)

int func(int *A, int *B) {
    return *A + *B;
}

int main() {
    int A = 3, B = 5;
    return func(&A, &B);
}
