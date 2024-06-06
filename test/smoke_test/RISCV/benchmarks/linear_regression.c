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

// NOTE: failing because vector instructions not supported yet 

// UNSUPPORTED: not-implemented
// REQUIRES: system-linux
// REQUIRES: riscv64-linux-gnu-gcc
// RUN: riscv64-linux-gnu-gcc -fno-stack-protector -o %t %s
// RUN: llvm-mctoll -d -debug %t --include-files=/usr/include/stdio.h,/usr/include/stdlib.h
// RUN: lli %t-dis.ll 4 122 | FileCheck %s
// CHECK: Points:
// CHECK: (0, 6), (3, 0), (7, 3), (3, 3)  
// CHECK: Results:
// CHECK: 	a    = 69.636364
// CHECK: 	b    = -0.363636
// CHECK: 	xbar = 51.250000
// CHECK: 	ybar = 51.000000
// CHECK: 	r2   = 0.181818
// CHECK: 	SX   = 205
// CHECK: 	SY   = 204
// CHECK: 	SXX  = 10531
// CHECK: 	SYY  = 10422
// CHECK: 	SXY  = 10446

#include <stdio.h>
#include <stdlib.h>

typedef struct {
    char x;
    char y;
} point_t;

void points_random_init(point_t *points, long size) {
    for (int i = 0; i < size; i++) {
        points[i].x = '0' + rand() % 10;
        points[i].y = '0' + rand() % 10;
    }
}

void points_display(point_t *points, long size) {
    for (int i = 0; i < size; i++) {
        printf("(%c, %c), ", points[i].x, points[i].y);
    }
    printf("\b\b  \n");
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "USAGE: %s <size> <seed>\n", argv[0]);
        exit(1);
    }

    long size = atol(argv[1]);
    if (size == 0) {
        fprintf(stderr, "linear_regression: invalid size\n");
        exit(EXIT_FAILURE);
    }

    int seed = atoi(argv[2]);
    if (seed == 0) {
        fprintf(stderr, "linear_regression: invalid seed\n");
        exit(EXIT_FAILURE);
    }

    point_t *points = (point_t *)malloc(size * sizeof(point_t));
    if (points == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    srand(seed);

    puts("Points:");
    points_random_init(points, size);
    points_display(points, size);

    long long n = size; 
    long long SX_ll = 0, SY_ll = 0, SXX_ll = 0, SYY_ll = 0, SXY_ll = 0;

    // ADD UP RESULTS
    for (long i = 0; i < n; i++)
    {
        //Compute SX, SY, SYY, SXX, SXY
        SX_ll  += points[i].x;
        SXX_ll += points[i].x * points[i].x;
        SY_ll  += points[i].y;
        SYY_ll += points[i].y * points[i].y;
        SXY_ll += points[i].x * points[i].y;
    }

    double a, b, xbar, ybar, r2;
    double SX = (double)SX_ll;
    double SY = (double)SY_ll;
    double SXX= (double)SXX_ll;
    double SYY= (double)SYY_ll;
    double SXY= (double)SXY_ll;

    b = (double)(n*SXY - SX*SY) / (n*SXX - SX*SX);
    a = (SY_ll - b*SX_ll) / n;
    xbar = (double)SX_ll / n;
    ybar = (double)SY_ll / n;
    r2 = (double)(n*SXY - SX*SY) * (n*SXY - SX*SY) / ((n*SXX - SX*SX)*(n*SYY - SY*SY));


    printf("Results:\n");
    printf("\ta    = %lf\n", a);
    printf("\tb    = %lf\n", b);
    printf("\txbar = %lf\n", xbar);
    printf("\tybar = %lf\n", ybar);
    printf("\tr2   = %lf\n", r2);
    printf("\tSX   = %lld\n", SX_ll);
    printf("\tSY   = %lld\n", SY_ll);
    printf("\tSXX  = %lld\n", SXX_ll);
    printf("\tSYY  = %lld\n", SYY_ll);
    printf("\tSXY  = %lld\n", SXY_ll);

    free(points);

    return 0;
}