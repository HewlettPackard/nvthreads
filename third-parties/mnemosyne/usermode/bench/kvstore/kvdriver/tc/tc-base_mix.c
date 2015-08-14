/*
    Copyright (C) 2011 Computer Sciences Department, 
    University of Wisconsin -- Madison

    ----------------------------------------------------------------------

    This file is part of Mnemosyne: Lightweight Persistent Memory, 
    originally developed at the University of Wisconsin -- Madison.

    Mnemosyne was originally developed primarily by Haris Volos
    with contributions from Andres Jaan Tack.

    ----------------------------------------------------------------------

    Mnemosyne is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation, version 2
    of the License.
 
    Mnemosyne is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, 
    Boston, MA  02110-1301, USA.

### END HEADER ###
*/

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <string.h>
#include <getopt.h>
#include <assert.h>
#include <sched.h>
#include <mnemosyne.h>
#include <mtm.h>
#include <pmalloc.h>
#include <itm.h>
#include "tcutil.h"
#include "tcbdb.h"
#include "ut_barrier.h"
#include "mix-bits.h"
#include "hashtable.h"
#include "elemset.h"
#include "tc_mix.h"
#include "common.h"
#include "tc_mix-bits.h"


int tc_base_mix_init(void *args)
{
	mix_global_state_t     *mix_global_state = NULL;
	tc_mix_global_state_t  *global_state = NULL;

	global_state = (tc_mix_global_state_t *) malloc(sizeof(tc_mix_global_state_t));
	mix_init(&mix_global_state, tc_base_op_ins, tc_base_op_del, tc_base_op_get, (void *) global_state);

	return tc_mix_init_internal(global_state, 1);
}


int 
tc_base_op_ins(void *t, void *elem, int lock)
{
	mix_global_state_t     *mix_global_state = (mix_global_state_t *) ubench_desc.global_state;
	tc_mix_global_state_t  *global_state = (tc_mix_global_state_t *) mix_global_state->data;
	mix_thread_state_t     *thread_state = (mix_thread_state_t *) t;
	TCBDB                  *bdb=global_state->bdb;
	object_t               *obj = (object_t *) elem;
	bool                   result;
	int                    key = obj->key;

	if (tcbdbput(bdb, &(obj->key), sizeof(int), &(obj->value), vsize) == false) {
		return -1;
	}
	tcbdbsync(bdb);
	return 0;
}


int 
tc_base_op_del(void *t, void *elem, int lock)
{
	mix_global_state_t     *mix_global_state = (mix_global_state_t *) ubench_desc.global_state;
	tc_mix_global_state_t  *global_state = (tc_mix_global_state_t *) mix_global_state->data;
	mix_thread_state_t     *thread_state = (mix_thread_state_t *) t;
	TCBDB                  *bdb=global_state->bdb;
	object_t               *obj = (object_t *) elem;

	if (tcbdbout(bdb, &(obj->key), sizeof(int)) == false) {
		return -1;
	}
	tcbdbsync(bdb);

	return 0;
}


int 
tc_base_op_get(void *t, void *elem, int lock)
{
	mix_global_state_t     *mix_global_state = (mix_global_state_t *) ubench_desc.global_state;
	tc_mix_global_state_t  *global_state = (tc_mix_global_state_t *) mix_global_state->data;
	mix_thread_state_t     *thread_state = (mix_thread_state_t *) t;
	TCBDB                  *bdb=global_state->bdb;
	object_t               *obj = (object_t *) elem;
	int                    vs;

	if (tcbdbget(bdb, &(obj->key), sizeof(int), &vs) == false) {
		return -1;
	}

	return 0;
}


void tc_mix_latency_thread_main(unsigned int tid)
{
	mix_thread_main(tid, 1, 0, 1);
}


void tc_mix_throughput_thread_main(unsigned int tid)
{
	mix_thread_main(tid, 0, 0, 1);
}
