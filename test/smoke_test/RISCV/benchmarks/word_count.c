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
// RUN: llvm-mctoll -d -debug %t --include-files=/usr/include/stdio.h,/usr/include/stdlib.h,/usr/include/string.h
// RUN: lli %t-dis.ll 10 | FileCheck %s
// CHECK: et: 6
// CHECK: sit: 3
// CHECK: amet: 3
// CHECK: non: 3
// CHECK: Ut: 3
// CHECK: ligula: 3
// CHECK: vel: 3
// CHECK: semper: 3
// CHECK: Praesent: 3
// CHECK: dolor: 2

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_WORDS 1024

typedef struct {
    char* word;
    int count;
} wc_count_t;

wc_count_t *words;

typedef struct {
    long flen;
    char *fdata;
} wc_data_t;

int compare_counts(const void *a, const void *b) {
    wc_count_t *word1 = (wc_count_t *)a;
    wc_count_t *word2 = (wc_count_t *)b;
    return word2->count - word1->count;
}

void init_words() {
    for (int i = 0; i < MAX_WORDS; i++) {
        words[i].word = NULL;
        words[i].count = 0;
    }
}

void bubble_sort(wc_count_t words[], int n) {
    for (int i = 0; i < n - 1; i++) {
        for (int j = 0; j < n - 1 - i; j++) {
            if (words[j].count < words[j + 1].count) {
                wc_count_t temp = words[j];
                words[j] = words[j + 1];
                words[j + 1] = temp;
            }
        }
    }
}

void word_count(char *data) {
    int word_count_index = 0;

    const char *DELIMITERS = " ,.\n";

    char *token = strtok(data, DELIMITERS);
    while (token != NULL) {
        // Check if the word already exists in the words array
        int i;
        for (i = 0; i < word_count_index; i++) {
            if (strcmp(words[i].word, token) == 0) {
                words[i].count++;
                break;
            }
        }

        // If word does not exist in the words array, add it
        if (i == word_count_index && word_count_index < MAX_WORDS) {
            words[word_count_index].word = strdup(token);
            words[word_count_index].count = 1;
            word_count_index++;
        }

        token = strtok(NULL, DELIMITERS);
    }

    // Sort the words by count, descending
    // FIXME: at the moment raising qsort results in an "Illegal instruction"
    // qsort(words, word_count_index, sizeof(wc_count_t), compare_counts);
    bubble_sort(words, word_count_index);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "USAGE: %s [top # of words]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int top_words = atoi(argv[1]);
    if (top_words == 0) {
        fprintf(stderr, "matrix_multiply: invalid size\n");
        exit(EXIT_FAILURE);
    }

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

    size_t size = strlen(DATA) + 1;
    char *data = (char *)malloc(size);
    strcpy(data, DATA);

    // wc_data_t *wc_data = (wc_data_t *)malloc(sizeof(wc_data_t));
    // wc_data->flen = size;
    // wc_data->fdata = data;

    words = (wc_count_t*)calloc(MAX_WORDS, sizeof(wc_count_t));
    init_words();
    word_count(data);

    // Show top N words
    for (int i = 0; i < top_words && words[i].word != NULL; i++) {
        printf("%s: %d\n", words[i].word, words[i].count);
    }

    // Free allocated memory
    for (int i = 0; i < MAX_WORDS && words[i].word != NULL; i++) {
        free(words[i].word);
    }
    free(data);

    return 0;
}
