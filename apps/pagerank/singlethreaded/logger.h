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

extern struct timespec* timer_alloc();

extern void timer_start(struct timespec *tmr);

extern l_time_t timer_total(struct timespec *tmr);

extern l_time_t timer_total_ms(struct timespec *tmr);

extern l_time_t timer_lap(struct timespec *tmr);

extern l_time_t timer_lap_ms(struct timespec *tmr);

extern void timer_free(struct timespec *tmr);

#endif // LOGGER_H
