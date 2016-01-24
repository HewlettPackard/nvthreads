/** Terry Hsu
 *  Microbenchmark
 *  Threads contending X% of the global array and measure
 *  runtime for pthread/nvthread/mnemosyne
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <stdint.h>
#include <time.h>
#include <sys/time.h>

//#ifdef ATLAS
#include "atlas_api.h"
#include "atlas_alloc.h"
//#endif

#ifdef MNEMOSYNE
#include <mnemosyne.h>
#include <mtm.h>
#include <pmalloc.h>
#endif

#define RANDOM

#define PAGE_MASK       (~(PAGE_SIZE-1))
#define PAGE_SIZE 4096
#define NPAGES 1000
#define nthreads 4

char *global_array;
size_t size;
uint64_t contend_size;
pthread_mutex_t lock;
int percent;
int cur;

// Only works for 5%
int fixedAddr(int tid){
	return (tid - 1) * (PAGE_SIZE / nthreads);
}

int generateRandomAddr(void){
	return rand()%PAGE_SIZE;
}

// Thread touches %x of array then return
void* thread_worker(void *args) {
	int tid = *(int *)args + 1;
	unsigned long i, cur, start, tmp, cur_page;
	cur_page = 0;

	fprintf(stderr, "thread %d ready to work\n", tid);
	while (cur_page < NPAGES) {
#ifdef RANDOM
		tmp = generateRandomAddr();
#else
		tmp = fixedAddr(tid);
#endif

		// Critical section
		//#ifdef MNEMOSYNE
		//        MNEMOSYNE_ATOMIC{
		//#elif INTEL_ICC
		//        __tm_atomic{
		//#else
		//        pthread_mutex_lock(&lock);
		//#endif
		NVM_LOCK(lock);
		start = cur_page * PAGE_SIZE + tmp;
		cur = start;
		i = 0;
		//      fprintf(stderr, "%d writing to page %d at byte %ld for %ld bytes (start: %ld)\n", tid, cur_page, tmp, contend_size, start);
		while (i < contend_size) {
			NVM_BEGIN_DURABLE();
			global_array[cur] = '1';
			NVM_STR2(global_array[cur], '1', sizeof(char)*8);
			NVM_END_DURABLE();
			i++;
			cur++;
			// Exceeds page boundry, reset pointer to the beginning of the page
			if ( cur > ((cur_page+1) * PAGE_SIZE) - 1 ) {
				cur = (cur_page * PAGE_SIZE);
				//              fprintf(stderr, "%d exceeds page boundary, reset to %ld\n", tid, cur);
			}
			//          fprintf(stderr, "%d writing %d-th byte to page %d at byte %ld\n", tid, i, cur_page, cur);
		}
		//      fprintf(stderr, "%d wrote %d bytes (start: %ld, end: %ld) to page %d (page range: %ld - %ld)\n",
		//      tid, i, start, cur, cur_page, (cur_page * PAGE_SIZE), ((cur_page+1) * PAGE_SIZE) - 1);
		//      fprintf(stderr, "----------------\n");
		//      fprintf(stderr, "%d done page %ld\n", tid, cur_page);
		//#ifdef MNEMOSYNE
		//        }
		//#elif INTEL_ICC
		//        }
		//#else
		//            pthread_mutex_unlock(&lock);
		//#endif
		NVM_UNLOCK(lock);

		cur_page++;
	}

	pthread_exit(NULL);
}

int pagedensity_rgn_id;
int main(int argc, char *argv[]) {

	//#ifdef ATLAS
	NVM_Initialize();
	pagedensity_rgn_id = NVM_FindOrCreateRegion("pagedensity", O_RDWR, NULL);
	//#endif
	// Set up global stuff
	srand(time(NULL));
	pthread_mutex_init(&lock, NULL);
	pthread_t tid[12];
	size = PAGE_SIZE * NPAGES;
	percent = atoi(argv[1]); // contend x% of the global array
	//  contend_size = (size / 100) * percent;
	contend_size = PAGE_SIZE * percent / 100;
	cur = -1; 
	int i = 0;

	// Timer
	struct timeval start, end;
	double elapsedTime;

	gettimeofday(&start, NULL);

	fprintf(stderr, "Writing to %d percent per page. Allocating %d pages, %zu bytes\n", percent, NPAGES, size);
	//#ifdef MNEMOSYNE
	//    global_array = (char *)pmalloc(size);
	//#else
	//    global_array = (char *)malloc(size);
	//#endif
	global_array = (char *)nvm_alloc(size, pagedensity_rgn_id);

	// Fork threads
	int thread_id[12];
	for (i = 0; i < nthreads; i++) {
		thread_id[i] = i;
		pthread_create(&tid[i], NULL, thread_worker, &thread_id[i]);
	}

	// Join
	for (i = 0; i < nthreads; i++) {
		pthread_join(tid[i], NULL);
	}

	//#ifdef MNEMOSYNE
	//    pfree(global_array);
	//#else
	//    free(global_array);
	//#endif

	gettimeofday(&end, NULL);
	elapsedTime = (end.tv_sec - start.tv_sec) * 1000.0;      // sec to ms
	elapsedTime += (end.tv_usec - start.tv_usec) / 1000.0;   // us to ms
	fprintf(stderr, "%f seconds\n", elapsedTime/1000);

	//#ifdef MNEMOSYNE
	//    char fn[1024];
	//    sprintf(fn, "build/examples/microbench/%dpercent_%dpages_mnemosyne.csv", percent, NPAGES);
	//    FILE *fp = fopen(fn, "a");
	//    fprintf(fp, "%f\n", elapsedTime/1000); // ms to seconds
	//    fclose(fp);
	//#endif
	NVM_CloseRegion(pagedensity_rgn_id);
	NVM_Finalize();

#ifdef NVM_STATS
	printf("Stats:\n");
    NVM_PrintNumFlushes();
    NVM_PrintStats();
#endif
	return 0;
}
