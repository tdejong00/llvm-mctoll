// REQUIRES: system-linux
// RUN: riscv64-linux-gnu-gcc -fno-stack-protector -o %t %s
// RUN: llvm-mctoll -d -debug %t --include-files=/usr/include/stdio.h,/usr/include/stdlib.h
// RUN: lli %t-dis.ll | FileCheck %s
// CHECK: H

#include <stdio.h>
#include <stdlib.h>

typedef struct node {
    char data;
    struct node *next;
} node_t;

void display(node_t *node) {
    while (node != NULL) {
        putchar(node->data);
        node = node->next;
    }
    putchar('\n');
}

int main(void) {
    node_t *head = (node_t *)malloc(sizeof(node_t));
    head->data = 'H';
    head->next = NULL;

    display(head);
    return 0;
}
