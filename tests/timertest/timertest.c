#include <pthread.h>
#include <unistd.h>

#include "logger.h"

void* worker(void* args) {
	sleep(1);
}

int main(int argc, char* argv) {
	l_clock_t* tmr = timer_alloc();
	
	size_t i;
	for(i = 0; i < 100; i++) {
		timer_start(tmr);
		
		printf(" TIMER_START-time=%lu\n", timer_total_ms(tmr));
		
		pthread_t thd;
		
		pthread_create(&thd, NULL, worker, NULL);
		
		pthread_join(thd, NULL);
		
		printf(" %d\t%lu ms\n", i, timer_total_ms(tmr));
	}
}
