/* Copyright (c) 2007-2009, Stanford University
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*        documentation and/or other materials provided with the distribution.
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

// NOTE: failing because incorrect output, reason unsure

// UNSUPPORTED: non-functional
// REQUIRES: system-linux
// REQUIRES: riscv64-unknown-linux-gnu-gcc
// RUN: riscv64-unknown-linux-gnu-gcc -fno-stack-protector -o %t %s
// RUN: llvm-mctoll -d -debug %t --include-files=/usr/include/stdio.h,/usr/include/stdlib.h,/usr/include/string.h,/usr/include/unistd.h
// RUN: lli %t-dis.ll | FileCheck %s
// CHECK: Dimension = 3
// CHECK: Number of clusters = 100
// CHECK: Number of points = 100000
// CHECK: Size of each dimension = 1000
// CHECK: Generating points
// CHECK: Generating means
// CHECK: Initializing clusters
// CHECK: 
// CHECK: 
// CHECK: Starting iterative algorithm
// CHECK: ..................................................................................................
// CHECK: 
// CHECK: Final Means:
// CHECK:   901   868   676 
// CHECK:   295   911   840 
// CHECK:   510   905   348 
// CHECK:   259   499   523 
// CHECK:   889   373   904 
// CHECK:   863   881    80 
// CHECK:   704   142   509 
// CHECK:    82   348   114 
// CHECK:   269   515   899 
// CHECK:   879    79   115 
// CHECK:   701   114   919 
// CHECK:   712   834   302 
// CHECK:   321   651   394 
// CHECK:   889   896   466 
// CHECK:   299   704   111 
// CHECK:   498   694   259 
// CHECK:   105   897   471 
// CHECK:   494   882   913 
// CHECK:   115   278   328 
// CHECK:   100   889   896 
// CHECK:   500   528   899 
// CHECK:   898   114   871 
// CHECK:   497   730   524 
// CHECK:   102   674   506 
// CHECK:   697   388   488 
// CHECK:   895   279   428 
// CHECK:   105   553   708 
// CHECK:   626   521   679 
// CHECK:   693   648   474 
// CHECK:   274   113   252 
// CHECK:   107   858   686 
// CHECK:   423   526    87 
// CHECK:   296    94   903 
// CHECK:   721   526   889 
// CHECK:   318   325   362 
// CHECK:   336   291   586 
// CHECK:   278   503   237 
// CHECK:   910   461   286 
// CHECK:   639   467    92 
// CHECK:   392   899   111 
// CHECK:   687   917   816 
// CHECK:   510   277   419 
// CHECK:   111   570    92 
// CHECK:    85   383   528 
// CHECK:   108   914   246 
// CHECK:   373   476   708 
// CHECK:   685   906   532 
// CHECK:   313   720   909 
// CHECK:   297   869   319 
// CHECK:   308   109   697 
// CHECK:   902    97   354 
// CHECK:   688   107   720 
// CHECK:   497    92   574 
// CHECK:   104   141   909 
// CHECK:   500   451   286 
// CHECK:   355   312   888 
// CHECK:   885   885   900 
// CHECK:   753   344   717 
// CHECK:    94   662   885 
// CHECK:   704   309   277 
// CHECK:   307    97   456 
// CHECK:   603    88    97 
// CHECK:   705    95   290 
// CHECK:   495   112   862 
// CHECK:   317   886   540 
// CHECK:    98   391   906 
// CHECK:   714   568   282 
// CHECK:   919   314   680 
// CHECK:   907   682   419 
// CHECK:   535   302   677 
// CHECK:   902   654   887 
// CHECK:   624   695    87 
// CHECK:   109   706   264 
// CHECK:   910   267   130 
// CHECK:   729   734   689 
// CHECK:   505   706   755 
// CHECK:   486   906   668 
// CHECK:    84    86   306 
// CHECK:   112   130   517 
// CHECK:   717   249    79 
// CHECK:   111   115    94 
// CHECK:   362    95    85 
// CHECK:   622   314   894 
// CHECK:   876   471    88 
// CHECK:   488   284   117 
// CHECK:   908   884   258 
// CHECK:   878   670   131 
// CHECK:   898    97   613 
// CHECK:   139   310   725 
// CHECK:   117   860    74 
// CHECK:   488   505   494 
// CHECK:   671   734   912 
// CHECK:    91   499   335 
// CHECK:   891   581   686 
// CHECK:   621   905   119 
// CHECK:   494   103   301 
// CHECK:   882   487   509 
// CHECK:   284   699   673 
// CHECK:    91    99   711 
// CHECK:   275   319   104

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define NUM_POINTS 100000
#define NUM_MEANS 100
#define DIM 3
#define GRID_SIZE 1000

int points[NUM_POINTS][DIM];
int means[NUM_MEANS][DIM];
int clusters[NUM_POINTS];
int modified = 0;

/** generate_points()
 *  Generate the points
 */
void generate_points() 
{
    for (int i = 0; i < NUM_POINTS; i++)
    {
        for (int j = 0; j < DIM; j++) 
        {
            points[i][j] = rand() % GRID_SIZE;
        }
    }
}

/** generate_means()
 *  Generate the means
 */
void generate_means() 
{
    for (int i = 0; i < NUM_MEANS; i++)
    {
        for (int j = 0; j < DIM; j++) 
        {
            means[i][j] = rand() % GRID_SIZE;
        }
    }
}

/** initialize_clusters()
 *  Initialize the clusters
 */
void initialize_clusters()
{
    for (int i = 0; i < NUM_POINTS; i++) {
        clusters[i] = -1;
    }
}

/** get_sq_dist()
 *  Get the squared distance between 2 points
 */
static inline unsigned int get_sq_dist(int *v1, int *v2)
{
    unsigned int sum = 0;
    for (int i = 0; i < DIM; i++) 
    {
        sum += ((v1[i] - v2[i]) * (v1[i] - v2[i])); 
    }
    return sum;
}

/** add_to_sum()
 *	Helper function to update the total distance sum
 */
void add_to_sum(int *sum, int *point)
{
    int i;
    
    for (i = 0; i < DIM; i++)
    {
        sum[i] += point[i];    
    }    
}

/** find_clusters()
 *  Find the cluster that is most suitable for a given set of points
 */
void find_clusters() 
{
    unsigned int min_dist, cur_dist;
    int min_idx;

    for (int i = 0; i < NUM_POINTS; i++) 
    {
        min_dist = get_sq_dist(points[i], means[0]);
        min_idx = 0; 
        for (int j = 1; j < NUM_MEANS; j++)
        {
            cur_dist = get_sq_dist(points[i], means[j]);
            if (cur_dist < min_dist) 
            {
                min_dist = cur_dist;
                min_idx = j;    
            }
        }
        
        if (clusters[i] != min_idx) 
        {
            clusters[i] = min_idx;
            modified = 1;
        }
    }    
}

/** calc_means()
 *  Compute the means for the various clusters
 */
void calc_means()
{
    int grp_size = 0;
    
    int *sum = (int *)malloc(sizeof(int) * DIM);
    
    for (int i = 0; i < NUM_MEANS; i++) 
    {
        memset(sum, 0, DIM * sizeof(int));
        
        grp_size = 0;
        
        for (int j = 0; j < NUM_POINTS; j++)
        {
            if (clusters[j] == i) 
            {
                add_to_sum(sum, points[j]);
                grp_size++;
            }    
        }
        
        for (int j = 0; j < DIM; j++)
        {
            //printf("div sum = %d, grp size = %d\n", sum[j], grp_size);
            if (grp_size != 0)
            { 
                means[i][j] = sum[j] / grp_size;
            }
        }         
    }

    free(sum);
}

/** dump_matrix()
 *  Helper function to print out the points
 */
void dump_matrix()
{
    for (int i = 0; i < NUM_MEANS; i++) 
    {
        for (int j = 0; j < DIM; j++)
        {
            printf("%5d ",means[i][j]);
        }
        printf("\n");
    }
}

/** 
* This application groups 'num_points' row-vectors (which are randomly
* generated) into 'num_means' clusters through an iterative algorithm - the 
* k-means algorith 
*/
int main(int argc, char **argv) 
{
    printf("Dimension = %d\n", DIM);
    printf("Number of clusters = %d\n", NUM_MEANS);
    printf("Number of points = %d\n", NUM_POINTS);
    printf("Size of each dimension = %d\n", GRID_SIZE); 

    printf("Generating points\n");
    generate_points();

    printf("Generating means\n");
    generate_means();

    printf("Initializing clusters\n");
    initialize_clusters();
    // memset(clusters, -1, sizeof(int) * NUM_POINTS);
    
    modified = 1;
    
    printf("\n\nStarting iterative algorithm\n");
    
    while (modified) 
    {
        modified = 0;
        printf(".");
        
        find_clusters();
        calc_means();
    }
    
    printf("\n\nFinal Means:\n");
    dump_matrix();

    return 0;  
}
