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
// RUN: lli %t-dis.ll | FileCheck %s
// CHECK:   83    86    77    15    93    35    86    92    49    21 
// CHECK:   62    27    90    59    63    26    40    26    72    36 
// CHECK:   11    68    67    29    82    30    62    23    67    35 
// CHECK:   29     2    22    58    69    67    93    56    11    42 
// CHECK:   29    73    21    19    84    37    98    24    15    70 
// CHECK:   13    26    91    80    56    73    62    70    96    81 
// CHECK:    5    25    84    27    36     5    46    29    13    57 
// CHECK:   24    95    82    45    14    67    34    64    43    50 
// CHECK:   87     8    76    78    88    84     3    51    54    99 
// CHECK:   32    60    76    68    39    12    26    86    94    39 
// CHECK:   83    86    77    15    93    35    86    92    49    21 
// CHECK:   62    27    90    59    63    26    40    26    72    36 
// CHECK:   11    68    67    29    82    30    62    23    67    35 
// CHECK:   29     2    22    58    69    67    93    56    11    42 
// CHECK:   29    73    21    19    84    37    98    24    15    70 
// CHECK:   13    26    91    80    56    73    62    70    96    81 
// CHECK:    5    25    84    27    36     5    46    29    13    57 
// CHECK:   24    95    82    45    14    67    34    64    43    50 
// CHECK:   87     8    76    78    88    84     3    51    54    99 
// CHECK:   32    60    76    68    39    12    26    86    94    39

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>

#define GRID_SIZE 100  // all values in the matrix are from 0 to this value 
#define NUM_ROWS 10
#define NUM_COLS 10

int matrix[NUM_ROWS][NUM_COLS];
int cov[NUM_ROWS][NUM_COLS];
int mean[GRID_SIZE];

/** dump_points()
 *  Print the values in the matrix to the screen
 */
void dump_points()
{
   int i, j;
   
   for (i = 0; i < NUM_ROWS; i++) 
   {
      for (j = 0; j < NUM_COLS; j++)
      {
         printf("%5d ",matrix[i][j]);
      }
      printf("\n");
   }
}

/** generate_points()
 *  Create the values in the matrix
 */
void generate_points() 
{   
   int i, j;
   
   for (i=0; i<NUM_ROWS; i++) 
   {
      for (j=0; j<NUM_COLS; j++) 
      {
         matrix[i][j] = rand() % GRID_SIZE;
      }
   }
}

/** calc_mean()
 *  Compute the mean for each row
 */
void calc_mean() {
   int i, j;
   int sum = 0;
   
   for (i = 0; i < NUM_ROWS; i++) {
      sum = 0;
      for (j = 0; j < NUM_COLS; j++) {
         sum += matrix[i][j];
      }
      mean[i] = sum / NUM_COLS;   
   }
}

/** calc_cov()
 *  Calculate the covariance
 */
void calc_cov() {
   int i, j, k;
   int sum;
   
   for (i = 0; i < NUM_ROWS; i++) {
      for (j = i; j < NUM_ROWS; j++) {
         sum = 0;
         for (k = 0; k < NUM_COLS; k++) {
            sum = sum + ((matrix[i][k] - mean[i]) * (matrix[j][k] - mean[j]));
         }
         cov[i][j] = cov[j][i] = sum/(NUM_COLS-1);
      }   
   }   
}


int main(int argc, char **argv) {
   //Generate random values for all the points in the matrix
   generate_points();
   
   // Print the points
   dump_points();
   
   // Compute the mean and the covariance
   calc_mean();
   calc_cov();
   
   
   dump_points();
   return 0;
}

