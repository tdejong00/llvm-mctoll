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
// REQUIRES: riscv64-unknown-linux-gnu-gcc
// RUN: riscv64-unknown-linux-gnu-gcc -fno-stack-protector -o %t %s
// RUN: llvm-mctoll -d -debug %t --include-files=/usr/include/stdio.h,/usr/include/stdlib.h,/usr/include/string.h
// RUN: lli %t-dis.ll semper | FileCheck %s
// CHECK: Key found!
// CHECK: Key found!

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define OFFSET 5

void compute_hashes(char* word, char* final_word) {
    int i;
    for(i = 0; i < strlen(word); i++) {
        final_word[i] = word[i] + OFFSET;
    }
    final_word[i] = '\0';
}

void string_match(char *data, char *key) {
    char *key_final = (char *)malloc(strlen(key) + 1);
    compute_hashes(key, key_final);

    const char *DELIMITERS = " ,.\n";

    char *token = strtok(data, DELIMITERS);
    while (token != NULL) {
        char *token_final = (char *)malloc(strlen(token) + 1);
        compute_hashes(token, token_final);

        if(!strcmp(key_final, token_final)) {
            puts("Key found!");
        }
        token = strtok(NULL, DELIMITERS);
        free(token_final);
    }

    free(key_final);
}

int main(int argc, char *argv[]) {
    const char *DATA = "Lorem ipsum dolor sit amet, consectetur adipiscing elit.\
Nulla dui lectus, vehicula id suscipit non, vestibulum eget neque. Ut massa \
ligula, bibendum vel egestas vel, gravida quis ligula. Curabitur suscipit quam \
et ante porta semper eu at augue. Praesent et auctor quam. Donec egestas tellus \
lacus, non malesuada nisl semper et. Praesent ullamcorper, est et interdum \
fermentum, felis nulla feugiat purus, quis eleifend neque turpis sit amet \
augue. Vestibulum ut dolor sodales, tincidunt enim eget, imperdiet enim. \
Nulla sit amet blandit sem. Donec consectetur bibendum vulputate. Suspendisse \
sagittis lacus nec libero vehicula elementum. Ut tortor elit, fringilla ut \
lectus et, scelerisque porttitor ligula. Praesent sed massa eu magna faucibus \
vestibulum in a est. Ut dignissim ex et luctus semper. Phasellus vel interdum \
leo, id ullamcorper lorem. Vivamus non nisi arcu. Duis feugiat viverra posuere.";

    if (argc < 2) {
        fprintf(stderr, "USAGE: %s [key]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *data = (char *)malloc(strlen(DATA) + 1);
    strcpy(data, DATA);

    char *key = argv[1];

    string_match(data, key);

    free(data);

    return 0;
}
