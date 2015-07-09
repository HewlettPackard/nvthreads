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

#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include "stddefines.h"

#define DEF_GRID_SIZE 100  // all values in the matrix are from 0 to this value 
#define DEF_NUM_ROWS 10
#define DEF_NUM_COLS 10

int num_rows;
int num_cols;
int grid_size;

/** parse_args()
 *  Parse the user arguments to determine the number of rows and colums
 */
void parse_args(int argc, char **argv) 
{
   int c;
   extern char *optarg;
   extern int optind;
   
   num_rows = DEF_NUM_ROWS;
   num_cols = DEF_NUM_COLS;
   grid_size = DEF_GRID_SIZE;
   
   while ((c = getopt(argc, argv, "r:c:s:")) != EOF) 
   {
      switch (c) {
         case 'r':
            num_rows = atoi(optarg);
            break;
         case 'c':
            num_cols = atoi(optarg);
            break;
         case 's':
            grid_size = atoi(optarg);
            break;
         case '?':
            printf("Usage: %s -r <num_rows> -c <num_cols> -s <max value>\n", argv[0]);
            exit(1);
      }
   }
   
   if (num_rows <= 0 || num_cols <= 0 || grid_size <= 0) {
      printf("Illegal argument value. All values must be numeric and greater than 0\n");
      exit(1);
   }

   printf("Number of rows = %d\n", num_rows);
   printf("Number of cols = %d\n", num_cols);
   printf("Max value for each element = %d\n", grid_size);   
}

/** dump_points()
 *  Print the values in the matrix to the screen
 */
void dump_points(int **vals, int rows, int cols)
{
   int i, j;
   
   for (i = 0; i < rows; i++) 
   {
      for (j = 0; j < cols; j++)
      {
         dprintf("%5d ",vals[i][j]);
      }
      dprintf("\n");
   }
}

/** generate_points()
 *  Create the values in the matrix
 */
void generate_points(int **pts, int rows, int cols) 
{   
   int i, j;
   
   for (i=0; i<rows; i++) 
   {
      for (j=0; j<cols; j++) 
      {
         pts[i][j] = rand() % grid_size;
      }
   }
}

/** calc_mean()
 *  Compute the mean for each row
 */
void calc_mean(int **matrix, int *mean) {
   int i, j;
   int sum = 0;
   
   for (i = 0; i < num_rows; i++) {
      sum = 0;
      for (j = 0; j < num_cols; j++) {
         sum += matrix[i][j];
      }
      mean[i] = sum / num_cols;   
   }
}

/** calc_cov()
 *  Calculate the covariance
 */
void calc_cov(int **matrix, int *mean, int **cov) {
   int i, j, k;
   int sum;
   
   for (i = 0; i < num_rows; i++) {
      for (j = i; j < num_rows; j++) {
         sum = 0;
         for (k = 0; k < num_cols; k++) {
            sum = sum + ((matrix[i][k] - mean[i]) * (matrix[j][k] - mean[j]));
         }
         cov[i][j] = cov[j][i] = sum/(num_cols-1);
      }   
   }   
}


int main(int argc, char **argv) {
   
   int i;
   int **matrix, **cov;
   int *mean;
   
   parse_args(argc, argv);   
   
   // Create the matrix to store the points
   matrix = (int **)malloc(sizeof(int *) * num_rows);
   for (i=0; i<num_rows; i++) 
   {
      matrix[i] = (int *)malloc(sizeof(int) * num_cols);
   }
   
   //Generate random values for all the points in the matrix
   generate_points(matrix, num_rows, num_cols);
   
   // Print the points
   dump_points(matrix, num_rows, num_cols);
   
   // Allocate Memory to store the mean and the covariance matrix
   mean = (int *)malloc(sizeof(int) * num_rows);
   cov = (int **)malloc(sizeof(int *) * num_rows);
   for (i=0; i<num_rows; i++) 
   {
      cov[i] = (int *)malloc(sizeof(int) * num_rows);
   }
   
   // Compute the mean and the covariance
   calc_mean(matrix, mean);
   calc_cov(matrix, mean, cov);
   
   
   dump_points(cov, num_rows, num_rows);
   return 0;
}
