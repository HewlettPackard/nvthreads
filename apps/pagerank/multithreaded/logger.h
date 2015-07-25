#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <limits.h>
#include <time.h>

/* --- LOG ---  */

typedef unsigned int logd_lvl_t;

static const logd_lvl_t LOGD_ALL = 0;
static const logd_lvl_t LOGD_L = 1;
static const logd_lvl_t LOGD_M = 500;
static const logd_lvl_t LOGD_H = 1000;
static const logd_lvl_t LOGD_NONE = UINT_MAX;

extern void logd_set_level(logd_lvl_t lvl);

extern void logd(logd_lvl_t lvl, const char *format, ...);

extern void logd_e(const char *format, ...);

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
