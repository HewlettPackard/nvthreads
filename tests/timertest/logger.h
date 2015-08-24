#ifndef LOGGER_H
#define LOGGER_H

//#define LOGD_USE_HISTORY

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <limits.h>
#include <time.h>

/* --- TIMER --- */

typedef long l_time_t;
typedef struct timespec l_clock_t;

extern l_clock_t* timer_alloc();

extern void timer_start(l_clock_t* tmr);

extern l_time_t timer_total(l_clock_t* tmr);

extern l_time_t timer_total_ms(l_clock_t* tmr);

extern l_time_t timer_lap(l_clock_t* tmr);

extern l_time_t timer_lap_ms(l_clock_t* tmr);

extern void timer_free(l_clock_t* tmr);

#endif // LOGGER_H
