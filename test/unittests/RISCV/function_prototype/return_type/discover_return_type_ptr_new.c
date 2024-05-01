// RUN: riscv64-linux-gnu-gcc -o %t %s
// RUN: llvm-mctoll -d -debug %t 2>&1 | FileCheck %s
// CHECK: declare dso_local ptr @create(i32 %0)

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
