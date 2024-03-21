// RUN: riscv64-linux-gnu-gcc -o %t %s
// RUN: llvm-mctoll -d -debug %t 2>&1 | FileCheck %s
// CHECK: declare dso_local void @func()

void func(void) {
    // nothing
}

int main(void) {
   func();
   return 0; 
}
