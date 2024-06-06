// NOTE: failing because passing structs as arguments is still unstable

// UNSUPPORTED: not-implemented
// REQUIRES: system-linux
// REQUIRES: riscv64-linux-gnu-gcc
// RUN: riscv64-linux-gnu-gcc -fno-stack-protector -o %t %s
// RUN: llvm-mctoll -d -debug %t -I /usr/include/stdio.h
// RUN: lli %t-dis.ll | FileCheck %s
// CHECK: ID: 123
// CHECK: Age: 45
// CHECK: Verified: 1

#include <stdio.h>

typedef struct {
    unsigned int id;
    unsigned int age;
    unsigned int verified;
} person_t;

void display(person_t person) {
    printf("ID: %u\n", person.id);
    printf("Age: %u\n", person.age);
    printf("Verified: %u\n", person.verified);
}

int main(void) {
    person_t person = { 123, 45, 1 };
    display(person);
    return 0;
}
