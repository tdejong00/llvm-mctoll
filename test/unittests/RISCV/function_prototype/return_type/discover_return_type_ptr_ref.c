// RUN: riscv64-linux-gnu-gcc -fno-stack-protector -o %t %s
// RUN: llvm-mctoll -d -debug %t 2>&1 | FileCheck %s
// CHECK: declare dso_local ptr @func()

int X = 3;

int *func(void) {
    return &X;
}

int main(void) {
    return *func();
}
