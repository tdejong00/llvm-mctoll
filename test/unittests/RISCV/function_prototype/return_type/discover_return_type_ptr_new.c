// REQUIRES: system-linux
// REQUIRES: riscv64-unknown-linux-gnu-gcc
// RUN: riscv64-unknown-linux-gnu-gcc -fno-stack-protector -o %t %s
// RUN: llvm-mctoll -d -debug -I /usr/include/stdlib.h %t 2>&1 | FileCheck %s
// CHECK: declare dso_local i64 @create(i32 %0)

#include <stdlib.h>

typedef struct node {
    char data;
    struct node *next;
} node_t;

node_t *create(char data) {
    node_t *node = (node_t *)malloc(sizeof(node_t));
    node->data = data;
    node->next = NULL;
    return node;
}

int main(void) {
    node_t *head = create('X');
    return 0;
}
