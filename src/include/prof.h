#ifndef __PROF_H__
#define __PROF_H__

/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

/*
 * @file   Prof.cpp
 * @brief  Used for profiling purpose.
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 * @author Charlie Curtsinger <http://www.cs.umass.edu/~charlie
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>

#ifdef ENABLE_PROFILING

typedef struct timeinfo {
    unsigned long low;
    unsigned long high;
} timeinfo_t;

#ifdef timeofday
#define TIMER(x) double x##_total; time_t x##_start
void start(time_t *start);
double stop(time_t *start);
#define START_TIMER(x) start(&global_data->stats.x##_start)
#define STOP_TIMER(x) global_data->stats.x##_total += stop(&global_data->stats.x##_start)
#define PRINT_TIMER(x) fprintf(stderr, " " #x " time: %.10fs\n", (double)global_data->stats.x##_total)

#else
#define TIMER(x) uint64_t x##_total; timeinfo_t x##_start
void start(struct timeinfo *ti);
double stop(struct timeinfo *begin, struct timeinfo *end);
static unsigned long long elapse2ms(double elapsed) {
    return (unsigned long long)elapsed;
}
#define START_TIMER(x) start(&global_data->stats.x##_start)
#define STOP_TIMER(x) global_data->stats.x##_total += elapse2ms(stop(&global_data->stats.x##_start, NULL))
#define PRINT_TIMER(x) fprintf(stderr, " " #x " time: %.10f seconds\n", (double)global_data->stats.x##_total/2666647000)
#endif

#define COUNTER(x) volatile uint64_t x##_count;
#define INC_COUNTER(x) global_data->stats.x##_count++
#define DEC_COUNTER(x) global_data->stats.x##_count--
#define ADD_COUNTER(x, y) global_data->stats.x##_count += y 
#define PRINT_COUNTER(x) fprintf(stderr, " " #x " count: %lu\n", global_data->stats.x##_count)
#define PRINT_LOG_COUNTER(x) fprintf(stderr, " logging: %lf ms\n", (double)(global_data->stats.x##_count * 1000) / CLOCKS_PER_SEC)
#define PRINT_LOGGER_TIMER(x) fprintf(stderr, " " #x " time: %.4fms\n", (double)global_data->stats.x##_total)

// For page density
#define COUNTER_ARRAY(x,size) volatile uint64_t x##_count[size];
#define INC_COUNTER_ARRAY(x, index) global_data->stats.x##_count[index]++
#define INIT_COUNTER_ARRAY(x, size){\
    for( int i = 0; i < size; i++ ) {\
        global_data->stats.x##_count[i] = 0;\
    }\
}

#define PRINT_COUNTER_ARRAY(x, size) {\
    for( int i = 0; i < size; i++ ) {\
        if( global_data->stats.x##_count[i] != 0 ) {\
            fprintf(stderr, #x"[%d]: %lu\n", i, global_data->stats.x##_count[i]);\
        }\
    }\
}
#define OUTPUT_COUNTER_ARRAY_FILE(x, size, fp){\
    for( int i = 0; i < size; i++ ) {\
        fprintf(fp, "%d, %lu\n", i, global_data->stats.x##_count[i]);\
    }\
}

struct runtime_stats {
    //size_t alloc_count;
    //size_t cleanup_size;
    TIMER(serial);
    TIMER(logging);
    TIMER(diff_calculation);
    TIMER(diff_logging);
    COUNTER(logtimer);  // * 1000 / CLOCKS_PER_SEC
    COUNTER(commit);
    COUNTER(twinpage);
    COUNTER(suspectpage);
    COUNTER(slowpage);
    COUNTER(dirtypage_modified);
    COUNTER(dirtypage_owned);
    COUNTER(lazypage);
    COUNTER(shorttrans);
    COUNTER(faults);
    COUNTER(transactions);
    COUNTER(dirtypage_inserted);
    COUNTER(loggedpages);
    COUNTER_ARRAY(pagedensity, 4097UL);
    COUNTER(pdcount);
    COUNTER(dummy);
};

#else

struct runtime_stats {
};

#define START_TIMER(x)
#define STOP_TIMER(x)
#define PRINT_TIMER(x)

#define INC_COUNTER(x)
#define DEC_COUNTER(x)
#define PRINT_COUNTER(x)
#define ADD_COUNTER(x, y)
#define PRINT_COUNTER(x)
#define PRINT_LOG_COUNTER(x)
#define PRINT_LOGGER_TIMER(x)

#endif
#endif
