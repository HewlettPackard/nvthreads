/*
  Copyright 2015-2016 Hewlett Packard Enterprise Development LP
  
  This program is free software; you can redistribute it and/or modify 
  it under the terms of the GNU General Public License, version 2 as 
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, 
  but WITHOUT ANY WARRANTY; without even the implied warranty of 
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
  See the GNU General Public License for more details.

  You should have received a copy of the GNU General Public License 
  along with this program; if not, write to the Free Software 
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

*/

/* Copyright (c) 2007, Stanford University
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*     * Neither the name of Stanford University nor the
*       names of its contributors may be used to endorse or promote products
*       derived from this software without specific prior written permission.
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

/* Author: Terry Hsu 
   Description: This is a modified version of Phoenix kmeans that implements crash recovery
                for pthreads and NVthreads.
   Modifications: 
            1. Change 2D arrays in means and clusters to 1D to reduce the amount of malloc call sites
            2. Corresponding helper functions for the 2D arrays are changed to 1D
            3. Insert dummy sync points for NVthreads to record dirty pages
            4. NVthreads touches all data in means and clusters before crash to speedup dirty page lookup
            5. Add helper functions for pthreads to persist data using FS
            6. Insert elapsed time measurements
            7. Add -a (abort) argument to specify which iteration to abort
            8. Add -r (recover) argument for pthreads to specify recovery run

*/
#if defined(ENABLE_DMP)
//#include "dmp.h"
#endif

#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <pthread.h>
#include <stdint.h>
#include <sys/time.h>
#include <errno.h>
#include <fcntl.h> 
#include <sys/mman.h>
#include "stddefines.h"
#include "MapReduceScheduler.h"
#ifdef NVTHREADS
#include "nvrecovery.h"
#endif

// Phoenix default input
#define DEF_NUM_POINTS 100000
#define DEF_NUM_MEANS 2000
#define DEF_DIM 3
#define DEF_GRID_SIZE 1000

// Medium input
//#define DEF_NUM_POINTS 10000000 // 10M
//#define DEF_NUM_MEANS 100 // 100 centers
//#define DEF_DIM 3
//#define DEF_GRID_SIZE 1000

// Larger input
//#define DEF_NUM_POINTS 50000000 // 50M points (page fault handler cannot allocat this many page records)
//#define DEF_NUM_MEANS 10  // 10 centers
//#define DEF_DIM 3   // #dimension
//#define DEF_GRID_SIZE 1000  // #grid size in each dimension

#define DEF_ABORT_AT 0
#define false 0
#define true 1

//uint64_t modified;

char buf1[4096];
bool crashed;
bool syncDummy;

#define MAX_DIM 4

pthread_mutex_t gm;
pthread_mutex_t dm; //dummy mutex
pthread_cond_t cond;

#ifndef NVTHREADS
#define nvmalloc(size, name) malloc(size)
#define nvrecover(dest, size, name) 0
#define isCrashed(void) 0
#endif


uint64_t num_threads;
uint64_t num_means; // number of clusters
uint64_t num_points;
uint64_t dim;       // Dimension of each vector

typedef struct {
    uint64_t start_idx;
    uint64_t num_pts;
    uint64_t sum[MAX_DIM]; //max size EDB FIX ME
                      // EDB all new below here
    uint64_t dim;
    uint64_t num_means;
    uint64_t num_points;
    uint64_t *points;
    uint64_t *means;
    uint64_t *clusters;
    uint64_t thread_id;            
} thread_arg;

struct dummy_struct {
    uint64_t *means;
    uint64_t *clusters;
    uint64_t *means_values;
    uint64_t *clusters_values;
    uint64_t *iterations;
    uint64_t *iterations_value;
};

void dump_clusters(uint64_t *val, uint64_t size){
    printf("clusters (size: %lu):\n", size);
    for(uint64_t i = 0; i < size; i++){
        if ( i % 20== 0) {
            printf("\n[%lu]\t", i);
        }
        printf("%lu\t", val[i]);
    }
}
/** dump_points()
 *  Helper function to print out the points
 */
void dump_points(uint64_t *vals, uint64_t rows, uint64_t dim) {
    uint64_t i, j;

    for (i = 0; i < rows; i++) {
        printf("[%lu] ", i);
        for (j = 0; j < dim; j++) {
            printf("%lu ", vals[i*dim + j]);
        }
        printf("\n");
    }
}

/** generate_points()
 *  Generate the points
 */
void generate_points(uint64_t *pts, uint64_t size, uint64_t dim, uint64_t grid_size) {
    uint64_t i, j, idx;
    for (i = 0; i < size; i++) {
        for (j = 0; j < dim; j++) {
            idx = (i * dim) + j;
            pts[idx] = rand() % grid_size;
        }
    }
}

/** get_sq_dist()
 *  Get the squared distance between 2 points
 */
inline uint64_t get_sq_dist(uint64_t *v1, uint64_t *v2, uint64_t dim) {
    uint64_t i;

    uint64_t sum = 0;
    for (i = 0; i < dim; i++) {
        sum += ((v1[i] - v2[i]) * (v1[i] - v2[i]));
        
    }
    return sum;
}

/** add_to_sum()
 *	Helper function to update the total distance sum
 */
void add_to_sum(uint64_t *sum, uint64_t *point, uint64_t dim) {
    uint64_t i;

    for (i = 0; i < dim; i++) {
        sum[i] += point[i];
    }
}

/** find_clusters()
 *  Find the cluster that is most suitable for a given set of points
 */
void* find_clusters(void *arg) {
    uint64_t modified = false;
    thread_arg *t_arg = (thread_arg *)arg;
    uint64_t i, j;
    uint64_t min_dist, cur_dist;
    uint64_t min_idx;
    uint64_t start_idx = t_arg->start_idx;
    uint64_t end_idx = start_idx + t_arg->num_pts;
    uint64_t dim = t_arg->dim;
    uint64_t num_means = t_arg->num_means;
    uint64_t num_points = t_arg->num_points;
    uint64_t *means = t_arg->means;
    uint64_t *points = t_arg->points;
    uint64_t *clusters = t_arg->clusters;

    for (i = start_idx; i < end_idx; i++) {
        min_dist = get_sq_dist(&points[i*dim], &means[0*dim], dim);
        min_idx = 0;
        for (j = 1; j < num_means; j++) {
            cur_dist = get_sq_dist(&points[i*dim], &means[j*dim], dim);
            if ( cur_dist < min_dist ) {
                min_dist = cur_dist;
                min_idx = j;
            }
        }

        if ( clusters[i] != min_idx ) {
            clusters[i] = min_idx;
            modified = true;
        }
    }
    return (void *)(intptr_t)modified;
}

/** calc_means()
 *  Compute the means for the various clusters
 */
void* calc_means(void *arg) {
    uint64_t i, j, grp_size;
    uint64_t *sum;
    thread_arg *t_arg = (thread_arg *)arg;
    uint64_t start_idx = t_arg->start_idx;
    uint64_t end_idx = start_idx + t_arg->num_pts;
    uint64_t dim = t_arg->dim;
    uint64_t num_points = t_arg->num_points;
    uint64_t *clusters = t_arg->clusters;
    uint64_t *means = t_arg->means;
    uint64_t *points = t_arg->points;

    sum = t_arg->sum;

    for (i = start_idx; i < end_idx; i++) {
        memset(sum, 0, dim * sizeof(uint64_t));
        grp_size = 0;

        for (j = 0; j < num_points; j++) {
            if ( clusters[j] == i ) {
                add_to_sum(sum, &points[j*dim], dim);
                grp_size++;
            }
        }
        
        for (j = 0; j < dim; j++) {
            if ( grp_size != 0 ) {
                means[(i*dim) + j] = sum[j] / grp_size;
            }
        }
    }
    return (void *)0;
}

void *touch_pages_function(void *args){
    struct dummy_struct *t_args = (struct dummy_struct*)args;
    uint64_t *means_values = t_args->means_values;
    uint64_t *clusters_values = t_args->clusters_values;
    uint64_t *iterations_value = t_args->iterations_value;
    uint64_t *means = t_args->means;
    uint64_t *clusters = t_args->clusters;
    uint64_t *iterations = t_args->iterations;

    printf("touching pages for means and clusters\n");
    memcpy(means, means_values, sizeof(uint64_t) * num_means * dim);
    memcpy(clusters, clusters_values, sizeof(uint64_t) * num_points);
    memcpy(iterations, iterations_value, sizeof(uint64_t));
    pthread_exit(NULL);
}

void *dummy_function(void* arg){
    pthread_mutex_lock(&dm);
    while (!syncDummy) {
        pthread_cond_wait(&cond, &dm);
    }
    fprintf(stderr, "dummy sync point\n");
    pthread_mutex_unlock(&dm);
    pthread_exit(NULL);
}

void pthread_persistent_data(void *data, size_t size, const char *filename){
    int fd = open(filename, O_WRONLY|O_CREAT, 0666);
    if ( fd == -1 ) {
        perror("can't open file\n");
        printf("file: %s\n", filename);
        abort();
    }
    ssize_t sz;
    sz = write(fd, (char*)data, size);
    if (sz != size) {
        perror("write error: ");
    }
    close(fd);
}

void pthread_recover_data(void *data, size_t size, const char *filename){
    int fd = open(filename, O_RDONLY, 0666);
    if ( fd == -1 ) {
        perror("can't read recover file\n");
        printf("file: %s\n", filename);
        abort();
    }
    off_t fsize = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
    printf("fsize %zu bytes\n",fsize);
    char *ptr = (char *)mmap(NULL, fsize, PROT_READ, MAP_PRIVATE, fd, 0);
    memcpy((char *)data, (char *)(ptr), fsize);

    close(fd);
}

int main(int argc, char **argv) {

    uint64_t grid_size; // size of each dimension of vector space
    uint64_t abort_at = -1; // abort at iteration
    uint64_t start_at = 0;
    uint64_t num_procs, curr_point;
    uint64_t i;
    pthread_t pid[256];
    pthread_attr_t attr;
    uint64_t num_per_thread, excess;
    double time_per_iteration;
    crashed = isCrashed();
    char means_filename[128];    
    char clusters_filename[128];    
    char iterations_filename[128];    
    bool recover_pthread = false;

    num_points = DEF_NUM_POINTS;
    num_means = DEF_NUM_MEANS;
    dim = DEF_DIM;
    grid_size = DEF_GRID_SIZE;
    abort_at = DEF_ABORT_AT;
    srand(0);
    struct timeval iteration_before, iteration_after, iteration_result;
    struct timeval main_before, main_after, main_result;
    struct timeval nvrecover_before, nvrecover_after, nvrecover_result;
    {
        int c;
        extern char *optarg;
        extern int optind;

        while ((c = getopt(argc, argv, "d:c:p:s:a:r:")) != EOF) {
            switch (c) {
            case 'd':
                dim = atoi(optarg);
                break;
            case 'c':
                num_means = atoi(optarg);
                break;
            case 'p':
                num_points = atoi(optarg);
                break;
            case 's':
                grid_size = atoi(optarg);
                break;
            case 'a':
                abort_at = atoi(optarg);
                break;
            case 'r':
                recover_pthread = true;
                break;
            case '?':
                printf("Usage: %s -d <vector dimension> -c <num clusters> -p <num points> -s <grid size> -a [abort at iteration a]\n", argv[0]);
                exit(1);
            }
        }

        if ( dim <= 0 || num_means <= 0 || num_points <= 0 || grid_size <= 0 ) {
            printf("Illegal argument value. All values must be numeric and greater than 0\n");
            exit(1);
        }
    }

    gettimeofday(&main_before, NULL);

    if (recover_pthread) {
        printf("Recover for pthread\n");
    }
    printf("Dimension = %lu\n", dim);
    printf("Number of clusters = %lu\n", num_means);
    printf("Number of points = %lu\n", num_points);
    printf("Size of each dimension = %lu\n", grid_size);
    printf("Abort at iteration = %lu\n", abort_at);
 	pthread_mutex_init(&gm, NULL);

    pthread_cond_init(&cond, NULL);
    pthread_mutex_init(&dm, NULL);
    pthread_t tid;

    sprintf(means_filename, "%s", "/mnt/ssd/tmp/means.txt");
    sprintf(clusters_filename, "%s", "/mnt/ssd/tmp/clusters.txt");
    sprintf(iterations_filename, "%s", "/mnt/ssd/tmp/iterations.txt");
    if (!recover_pthread) {
        unlink(means_filename);
        unlink(clusters_filename);
        unlink(iterations_filename);
    }

    /// Points (1-D)
    uint64_t *points = (uint64_t*)malloc(sizeof(uint64_t) * num_points * dim);
    generate_points(points, num_points, dim, grid_size);

    // For dummy sync point
    if (!crashed && !recover_pthread) {
        syncDummy = false;
        pthread_create(&tid, NULL, dummy_function, NULL);
    }
    else{
        gettimeofday(&nvrecover_before, NULL);     
    }

    /// Means (1-D)
    uint64_t *means = (uint64_t*)nvmalloc(sizeof(uint64_t) * num_means * dim, (char*)"means");
    if (crashed) {
        nvrecover(means, sizeof(uint64_t) * num_means * dim, (char*)"means");
    } else if (recover_pthread) {
        pthread_recover_data(means, sizeof(uint64_t) * num_means * dim, means_filename);
    } else {
        generate_points(means, num_means, dim, grid_size);
    }
//  dump_points(means, num_means, dim);
    uint64_t *saved_means = (uint64_t*)malloc(sizeof(uint64_t) * num_means * dim);

    /// Clusters
    uint64_t *clusters = (uint64_t *)nvmalloc(sizeof(uint64_t) * num_points, (char*)"clusters");
    if (crashed) {
        nvrecover(clusters, sizeof(uint64_t) * num_points, (char*)"clusters");
    } else if (recover_pthread) {
        pthread_recover_data(clusters, sizeof(uint64_t) * num_points, clusters_filename);
    } else{
        memset(clusters, -1, sizeof(uint64_t) * num_points);
    }
//  dump_clusters(clusters, num_points);
    uint64_t *saved_clusters = (uint64_t *)malloc(sizeof(uint64_t) * num_points);
    
    /// Iterations    
    uint64_t *iterations = (uint64_t *)nvmalloc(sizeof(uint64_t), (char*)"iterations");
    if (crashed) {
        nvrecover(iterations, sizeof(uint64_t), (char*)"iterations");
        start_at = (*iterations);
        printf("starts from iteration %lu\n", *iterations);
    } else if (recover_pthread) {
        pthread_recover_data(iterations, sizeof(uint64_t), iterations_filename);
        start_at = (*iterations);
        printf("starts from iteration %lu\n", *iterations);
    } else{
        (*iterations) = 0;
        printf("init iterations to %lu\n", *iterations);
    }
    uint64_t *saved_iterations = (uint64_t *)malloc(sizeof(uint64_t));

    if (!crashed && !recover_pthread) {
        pthread_mutex_unlock(&dm);
        syncDummy = true;
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&dm);
        pthread_join(tid, NULL);
    }
    else{
        gettimeofday(&nvrecover_after, NULL);
        timersub(&nvrecover_after, &nvrecover_before, &nvrecover_result);
        printf("nvrecover time: ------------[%ld.%06ld]--------------\n", (long int)nvrecover_result.tv_sec, (long int)nvrecover_result.tv_usec);
     }

    pthread_attr_init(&attr);
    pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
    CHECK_ERROR((num_procs = sysconf(_SC_NPROCESSORS_ONLN)) <= 0); // FIX ME
                                                                   //CHECK_ERROR((num_procs = 4 * sysconf(_SC_NPROCESSORS_ONLN)) <= 0); // FIX ME

    //   CHECK_ERROR( (pid = (pthread_t *)malloc(sizeof(pthread_t) * num_procs)) == NULL);


    uint64_t modified = true;

    /* Create the threads to process the distances between the various
    points and repeat until modified is no longer valid */
    thread_arg arg[256];
    time_per_iteration = 0;
    while (modified) {
        gettimeofday(&iteration_before, NULL);
        
        num_per_thread = num_points / num_procs;
        excess = num_points % num_procs;
        modified = false;
        dprintf(".");
        curr_point = 0;
        num_threads = 0;
        
        if (abort_at != 0 && (*iterations) == abort_at) {
            // quick hack: touch dirty pages
            memcpy(saved_means, means, sizeof(uint64_t) * num_means * dim);
            memcpy(saved_clusters, clusters, sizeof(uint64_t) * num_points);
            memcpy(saved_iterations, iterations, sizeof(uint64_t));

            // Set means and clusters to 0 (dirty it!)
            memset(means, 0, sizeof(uint64_t) * num_means * dim);
            memset(clusters, 0, sizeof(uint64_t) * num_points);
            (*iterations) = 0;

            struct dummy_struct dummy_args;
            dummy_args.means_values = saved_means;
            dummy_args.clusters_values = saved_clusters;
            dummy_args.iterations_value = saved_iterations;
            dummy_args.means = means;
            dummy_args.clusters = clusters;
            dummy_args.iterations = iterations;
            // Ask child thread to restore the values in means and cluster
            pthread_create(&tid, NULL, touch_pages_function, (void*) &dummy_args);
            pthread_join(tid, NULL);
 
            
//          dump_points(means, num_means, dim);
//          dump_clusters(clusters, num_points);
            printf("intentionally abort!\n");
            gettimeofday(&main_after, NULL);
            timersub(&main_after, &main_before, &main_result);
            time_per_iteration = (double) time_per_iteration / (*iterations);
            printf("Time (Per iteration):\t-------------%lf-------------\n", time_per_iteration);
            printf("Time (Start to crash):\t-------------[%ld.%06ld]------------\n", (long int)main_result.tv_sec, (long int)main_result.tv_usec);
//          dump_points(means, num_means, dim);
//          dump_clusters
            abort();
        }

        while (curr_point < num_points) {
            //	CHECK_ERROR((arg = (thread_arg *)malloc(sizeof(thread_arg))) == NULL);
            arg[num_threads].start_idx = curr_point;
            arg[num_threads].num_pts = num_per_thread;
            arg[num_threads].dim = dim;
            arg[num_threads].num_means = num_means;
            arg[num_threads].num_points = num_points;
            arg[num_threads].means = means;
            arg[num_threads].points = points;
            arg[num_threads].clusters = clusters;

            if ( excess > 0 ) {
                arg[num_threads].num_pts++;
                excess--;
            }
            curr_point += arg[num_threads].num_pts;
            num_threads++;
        }

//      printf("in this run, num_threads is %lu, num_per_thread is %lu\n", num_threads, num_per_thread);

        for (i = 0; i < num_threads; i++) {
            arg[i].thread_id = i + 1;            
            CHECK_ERROR((pthread_create(&(pid[i]), &attr, find_clusters,
                                        (void *)(&arg[i]))) != 0);
            // EDB - with hierarchical commit we would not have had to
            // "localize" num_threads.
        }

        // Log iteration, must be inbetween sync point
//      printf("iteration %lu\n", *iterations);
        (*iterations)++;

        assert(num_threads == num_procs);
        for (i = 0; i < num_threads; i++) {
            void *rt = 0;
            uint64_t m;
            pthread_join(pid[i], &rt);
            m = (uint64_t)(intptr_t)rt;
            modified |= m;
        }

        num_per_thread = num_means / num_procs;
        excess = num_means % num_procs;
        curr_point = 0;
        num_threads = 0;

        assert(dim <= MAX_DIM);

        while (curr_point < num_means) {
            //	CHECK_ERROR((arg = (thread_arg *)malloc(sizeof(thread_arg))) == NULL);
            arg[num_threads].start_idx = curr_point;
            //	arg[num_threads].sum = (uint64_t *)malloc(dim * sizeof(uint64_t));
            arg[num_threads].num_pts = num_per_thread;
            if ( excess > 0 ) {
                arg[num_threads].num_pts++;
                excess--;
            }
            curr_point += arg[num_threads].num_pts;
            num_threads++;
        }

        for (i = 0; i < num_threads; i++) {
//          CHECK_ERROR((pthread_create(&(pid[i]), &attr, calc_means,
//                                      (void *)(&arg[i]))) != 0);
            arg[i].thread_id = i + 1;
            pthread_create(&(pid[i]), &attr, calc_means,
                                        (void *)(&arg[i]));
        }

        assert(num_threads == num_procs);
        for (i = 0; i < num_threads; i++) {
            pthread_join(pid[i], NULL);
            //	 free (arg[i].sum);
        }

        gettimeofday(&iteration_after, NULL);
        timersub(&iteration_after, &iteration_before, &iteration_result);
        time_per_iteration = time_per_iteration + (double)iteration_result.tv_sec + (double)(iteration_result.tv_usec * 0.000001);
//      printf("iteration %lu\t%ld.%06ld\n", *iterations, (long int)iteration_result.tv_sec, (long int)iteration_result.tv_usec);

        // Pthreads persistent data 
#ifdef PTHREADS
        pthread_persistent_data(means, sizeof(uint64_t) * num_means * dim, means_filename);
        pthread_persistent_data(clusters,  sizeof(uint64_t) * num_points, clusters_filename);
        pthread_persistent_data(iterations, sizeof(*iterations), iterations_filename);
#endif
    }

//  printf("\n\nFinal means:\n");
//  dump_points(means, num_means, dim);

    free(points);
    free(means);
    free(clusters);

    gettimeofday(&main_after, NULL);
    timersub(&main_after, &main_before, &main_result);
    time_per_iteration = (double) time_per_iteration / (*iterations);
    printf("-------------------\n");
    printf("Kmeans done\n");
    printf("#points: %lu\n", num_points);
    printf("Abort at iteration: %lu\n", abort_at);
    printf("Start from iteration: %lu\n", start_at);
    printf("Final iterations: %lu\n", *iterations);
    printf("Time (Per iteration):\t-------------%lf-------------\n", time_per_iteration);
    if (abort_at == 0) {
        printf("Time (Start to end):\t-------------[%ld.%06ld]------------\n", (long int)main_result.tv_sec, (long int)main_result.tv_usec);
    } else{
        printf("Time (Recover to end):\t-------------[%ld.%06ld]------------\n", (long int)main_result.tv_sec, (long int)main_result.tv_usec);
    }
    printf("-------------------\n");
    return 0;
}
