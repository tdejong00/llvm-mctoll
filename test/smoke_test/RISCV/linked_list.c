// REQUIRES: system-linux
// REQUIRES: riscv64-linux-gnu-gcc
// RUN: riscv64-linux-gnu-gcc -fno-stack-protector -o %t %s
// RUN: llvm-mctoll -d -debug %t --include-files=/usr/include/stdio.h,/usr/include/stdlib.h
// RUN: lli %t-dis.ll | FileCheck %s
// CHECK: HELLO!

#include <stdio.h>
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

void display(node_t *node) {
    while (node != NULL) {
        putchar(node->data);
        node = node->next;
    }
    putchar('\n');
}

void add(node_t *node, char data) {
    while(node->next != NULL) {
        node = node->next;
    }

    node->next = create(data);
}

int main(void) {
    node_t *head = create('H');
    add(head, 'E');
    add(head, 'L');
    add(head, 'L');
    add(head, 'O');
    add(head, '!');
    display(head);
    return 0;
}
