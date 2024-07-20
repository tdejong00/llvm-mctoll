// REQUIRES: system-linux
// REQUIRES: riscv64-unknown-linux-gnu-gcc
// RUN: riscv64-unknown-linux-gnu-gcc -fno-stack-protector -o %t %s
// RUN: llvm-mctoll -d -debug %t --include-files=/usr/include/stdio.h,/usr/include/stdlib.h
// RUN: lli %t-dis.ll | FileCheck %s
// CHECK: No loop, constant indices:
// CHECK: 0: X = 1, Y = 10, Z = 100
// CHECK: 1: X = 2, Y = 20, Z = 200
// CHECK: 2: X = 3, Y = 30, Z = 300
// CHECK: 3: X = 4, Y = 40, Z = 400
// CHECK: 4: X = 5, Y = 50, Z = 500
// CHECK: Loop without retrieve element:
// CHECK: 0: X = 1, Y = 10, Z = 100
// CHECK: 1: X = 2, Y = 20, Z = 200
// CHECK: 2: X = 3, Y = 30, Z = 300
// CHECK: 3: X = 4, Y = 40, Z = 400
// CHECK: 4: X = 5, Y = 50, Z = 500
// CHECK: Loop with retrieve element:
// CHECK: 0: X = 1, Y = 10, Z = 100
// CHECK: 1: X = 2, Y = 20, Z = 200
// CHECK: 2: X = 3, Y = 30, Z = 300
// CHECK: 3: X = 4, Y = 40, Z = 400
// CHECK: 4: X = 5, Y = 50, Z = 500

#include <stdio.h>
#include <stdlib.h>

typedef struct { int x; int y; int z; } my_struct_t;

my_struct_t *func(int n) {
    my_struct_t *my_structs = (my_struct_t *)calloc(n, sizeof(my_struct_t));

    for (int i = 0; i < n; i++) {
        my_structs[i].x = i + 1;
        my_structs[i].y = (i + 1) * 10;
        my_structs[i].z = (i + 1) * 100;
    }

    return my_structs;
}

int main(void) {
    int n = 5;
    my_struct_t *my_structs = func(n);

    puts("No loop, constant indices:");
    printf("%d: X = %d, Y = %d, Z = %d\n", 0, my_structs[0].x, my_structs[0].y, my_structs[0].z);
    printf("%d: X = %d, Y = %d, Z = %d\n", 1, my_structs[1].x, my_structs[1].y, my_structs[1].z);
    printf("%d: X = %d, Y = %d, Z = %d\n", 2, my_structs[2].x, my_structs[2].y, my_structs[2].z);
    printf("%d: X = %d, Y = %d, Z = %d\n", 3, my_structs[3].x, my_structs[3].y, my_structs[3].z);
    printf("%d: X = %d, Y = %d, Z = %d\n", 4, my_structs[4].x, my_structs[4].y, my_structs[4].z);

    puts("Loop without retrieve element:");
    for (int i = 0; i < n; i++) {
        printf("%d: X = %d, Y = %d, Z = %d\n", i, my_structs[i].x, my_structs[i].y, my_structs[i].z);
    }

    puts("Loop with retrieve element:");
    for (int i = 0; i < n; i++) {
        my_struct_t my_struct = my_structs[i];
        printf("%d: X = %d, Y = %d, Z = %d\n", i, my_struct.x, my_struct.y, my_struct.z);
    }

    free(my_structs);

    return 0;
}