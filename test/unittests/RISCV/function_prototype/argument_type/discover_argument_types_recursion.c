// RUN: riscv64-linux-gnu-gcc -fno-stack-protector -o %t %s
// RUN: llvm-mctoll -d -debug %t 2>&1 | FileCheck %s
// CHECK: declare dso_local i32 @factorial(i32 %0)

int factorial(int N) {
  if (N == 0) {
    return 1;
  }
  return N * factorial(N - 1);
}

int main(void) {
    factorial(5);
    return 0;
}
