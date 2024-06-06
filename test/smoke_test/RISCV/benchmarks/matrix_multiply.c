/* Copyright (c) 2007-2009, Stanford University
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*     * Neither the name of Stanford University nor the names of its 
*       contributors may be used to endorse or promote products derived from 
*       this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY STANFORD UNIVERSITY ``AS IS'' AND ANY
* EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL STANFORD UNIVERSITY BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/ 

// REQUIRES: system-linux
// REQUIRES: riscv64-linux-gnu-gcc
// RUN: riscv64-linux-gnu-gcc -fno-stack-protector -o %t %s
// RUN: llvm-mctoll -d -debug %t --include-files=/usr/include/stdio.h,/usr/include/stdlib.h
// RUN: lli %t-dis.ll 4 123 | FileCheck %s
// CHECK: A:
// CHECK: 3 3 3 0
// CHECK: 9 1 5 2
// CHECK: 6 1 4 8
// CHECK: 1 1 6 1
// CHECK: B:
// CHECK: 9 7 6 3
// CHECK: 6 5 9 1
// CHECK: 6 8 6 8
// CHECK: 5 2 5 8
// CHECK: C:
// CHECK: 0 0 0 0
// CHECK: 0 0 0 0
// CHECK: 0 0 0 0
// CHECK: 0 0 0 0
// CHECK: Result:
// CHECK: 63 60 63 36
// CHECK: 127 112 103 84
// CHECK: 124 95 109 115
// CHECK: 56 62 56 60

#include <stdio.h>
#include <stdlib.h>

#define BLOCK_SIZE 64

void matrix_multiply(int *M1, int *M2, int *M3, int size) {
    for (int i = 0; i < size; i += BLOCK_SIZE)
    for (int j = 0; j < size; j += BLOCK_SIZE)
    for (int k = 0; k < size; k += BLOCK_SIZE)
    {
        for (int a = i; a < i + BLOCK_SIZE && a < size; a++)
        for (int b = i; b < i + BLOCK_SIZE && b < size; b++)
        for (int c = i; c < i + BLOCK_SIZE && c < size; c++)
        {
            M3[size * a + b] += M1[size * a + c] * M2[size * c + b];
        }
    }
}

void matrix_random_init(int *M, int size) {
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            M[i * size + j] = rand() % 10;
        }
    }
}

void matrix_zero_init(int *M, int size) {
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            M[i * size + j] = 0;
        }
    }
}

void matrix_display(int *M, int size) {
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            printf("%d ", M[i * size + j]);
        }
        printf("\n");
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "USAGE: %s [size] [seed]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int size = atoi(argv[1]);
    if (size == 0) {
        fprintf(stderr, "matrix_multiply: invalid size\n");
        exit(EXIT_FAILURE);
    }

    int seed = atoi(argv[2]);
    if (seed == 0) {
        fprintf(stderr, "matrix_multiply: invalid seed\n");
        exit(EXIT_FAILURE);
    }

    int *A = (int *)malloc(size * size * sizeof(int));
    if (A == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    int *B = (int *)malloc(size * size * sizeof(int));
    if (B == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    int *C = (int *)malloc(size * size * sizeof(int));
    if (C == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    srand(seed);

    puts("A:");
    matrix_random_init(A, size);
    matrix_display(A, size);

    puts("B:");
    matrix_random_init(B, size);
    matrix_display(B, size);

    puts("C:");
    matrix_zero_init(C, size);
    matrix_display(C, size);

    puts("Result:");
    matrix_multiply(A, B, C, size);
    matrix_display(C, size);

    free(A);
    free(B);
    free(C);
    return 0;
}
