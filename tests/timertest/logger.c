#include "logger.h"

/* --- TIMER ---  */
const l_time_t _TMR_S_IN_NS = 1000000000L;
const l_time_t _TMR_NS_IN_MS = 1000000L; 
const clockid_t _TMR_T = CLOCK_REALTIME;

extern struct timespec* timer_alloc() {
	return (struct timespec*)malloc(sizeof(struct timespec));
}

extern void timer_start(struct timespec *tmr) {
	clock_gettime(_TMR_T, tmr);
}

extern l_time_t timer_total(struct timespec *tmr) {
	struct timespec tmr_end;
	
	clock_gettime(_TMR_T, &tmr_end);
	
	return _TMR_S_IN_NS * (tmr_end.tv_sec - tmr->tv_sec) + tmr_end.tv_nsec - tmr->tv_nsec;
}

extern l_time_t timer_total_ms(struct timespec *tmr) {
	return timer_total(tmr) / _TMR_NS_IN_MS;
}

extern l_time_t timer_lap(struct timespec *tmr) {
	l_time_t time = timer_total(tmr);
	
	clock_gettime(_TMR_T, tmr);
	
	return time;
}

extern l_time_t timer_lap_ms(struct timespec *tmr) {
	return timer_lap(tmr) / _TMR_NS_IN_MS;
}

extern void timer_free(struct timespec *tmr) {
	free(tmr);
}
