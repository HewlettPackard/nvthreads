#include <pthread.h>
#include <unistd.h>

#include "logger.h"

void* worker(void* args) {
	printf("Spawned!\n");
	sleep(1);
}

int main(int argc, char* argv) {
	l_clock_t* tmr = timer_alloc();
	
	size_t i;
	for(i = 0; i < 100; i++) {
		timer_start(tmr);
		
		//sleep(1);
		pthread_t thd;
		
		pthread_create(&thd, NULL, worker, NULL);
		
		pthread_join(thd, NULL);
		
		printf(" %d\t%lu ms\n", i, timer_total_ms(tmr));
	}
}
