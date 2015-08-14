/* mtest.c - memory-mapped database tester/toy */
/*
 * Copyright 2011 Howard Chu, Symas Corp.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 *
 * A copy of this license is available in the file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 */
#define _XOPEN_SOURCE 500		/* srandom(), random() */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "mdb.h"
#include <sys/time.h>
#include <assert.h>
#include <pthread.h>


#define NUM_READERS 5
#define WRITER_STEPS 5
#define READER_REPS 10

typedef struct DB_Info
{
	MDB_env *env;
        int count;
        int* values;
} DB_Info;

void* add_key_values(void* ptr)
{
        
	char sval[32];
	int i = 0, j = 0, k = 0, rc, count, *values;
	MDB_env *env;
	MDB_dbi dbi;
	MDB_val key, data;
	MDB_txn *txn;
	MDB_stat mst;

        DB_Info* info = (DB_Info*) ptr;
	env = info->env;
        count = info->count;
        values = info->values;
	
	key.mv_size = sizeof(int);
	key.mv_data = sval;
	data.mv_size = sizeof(sval);
	data.mv_data = sval;

	printf("Adding %d values\n", count);
	for (j = 0; j < 5; j++)   
	{
		txn = NULL;
		rc = mdb_txn_begin(env, NULL, 0, &txn);
                if (j == 0)
			rc = mdb_open(txn, NULL, 0, &dbi);
        	for (i = j; i < count; i += 5) {	
			sprintf(sval, "%03x %d foo bar", values[i], values[i]);
			rc = mdb_put(txn, dbi, &key, &data, MDB_NOOVERWRITE);
			if (rc) k++;
		}
		printf("Writer step: %d\n", j);
        if (k) printf("%d duplicates skipped\n", k);
		rc = mdb_txn_commit(txn);
		rc = mdb_env_stat(env, &mst);

	}
	printf("Finished adding %d values\n", count);
#ifdef NVM_STATS
       NVM_PrintNumFlushes();
#endif
	return NULL;
}


void* delete_key_values(void* ptr)
{
	char sval[32];
	int i = 0, j = 0, rc, count, *values;
	MDB_env *env;
	MDB_dbi dbi;
	MDB_val key;
	MDB_txn *txn;

        DB_Info* info = (DB_Info*) ptr;
	env = info->env;
        count = info->count;
        values = info->values;
	
	key.mv_size = sizeof(int);
	key.mv_data = sval;

	for (i= count - 1; i > -1; i-= (random()%5)) {	
		j++;
		txn=NULL;
		rc = mdb_txn_begin(env, NULL, 0, &txn);
		rc = mdb_open(txn, NULL, 0, &dbi);
		sprintf(sval, "%03x ", values[i]);
		rc = mdb_del(txn, dbi, &key, NULL);
		if (rc) {
			j--;
			mdb_txn_abort(txn);
		} else {
			rc = mdb_txn_commit(txn);
		}
	}
	printf("Finished deleting %d items\n", j);

	return NULL;
}

void* read_key_values(void* ptr)
{
        int i=0, rc;
	MDB_env *env;
	MDB_dbi dbi;
	MDB_val key, data;
	MDB_txn *txn;
        MDB_cursor *cursor;
	MDB_stat mst;

        DB_Info* info = (DB_Info*) ptr;
	env = info->env;
	
	//key.mv_size = sizeof(int);
	//data.mv_size = sizeof(sval);

        for (i = 0; i < READER_REPS; i++)
        {
		rc = mdb_env_stat(env, &mst);
		rc = mdb_txn_begin(env, NULL, 1, &txn);
		rc = mdb_open(txn, NULL, 0, &dbi);
		rc = mdb_cursor_open(txn, dbi, &cursor);
//		printf("Cursor next\n");
		while ((rc = mdb_cursor_get(cursor, &key, &data, MDB_NEXT)) == 0) {
//			printf("key: %.*s, data: %.*s\n",
//				(int) key.mv_size,  (char *) key.mv_data,
//				(int) data.mv_size, (char *) data.mv_data);
		}
//		printf("Cursor prev\n");
		while ((rc = mdb_cursor_get(cursor, &key, &data, MDB_PREV)) == 0) {
//			printf("key: %.*s, data: %.*s\n",
//				(int) key.mv_size,  (char *) key.mv_data,
//				(int) data.mv_size, (char *) data.mv_data);
		}
		mdb_cursor_close(cursor);
		mdb_txn_abort(txn);
	}
	return NULL;
}

int main(int argc,char * argv[])
{

	int *values, rc, i, count;
	pthread_t readers[NUM_READERS], writer, eraser;
	
	MDB_env *env;

	struct timeval tv_start;
	struct timeval tv_end;

	srandom(time(NULL));

	//count = (random()%384) + 64;
	count = 3000; 

	values = (int *)malloc(count*sizeof(int));

	for(i = 0; i < count; i++) {
		values[i] = random()%1024;
	}

        assert(!gettimeofday(&tv_start, NULL));
	
        rc = mdb_env_create(&env);
	rc = mdb_env_set_mapsize(env, 10485760);
	//rc = mdb_env_open(env, "./testdb", MDB_FIXEDMAP |MDB_NOSYNC, 0664);
	rc = mdb_env_open(env, "./testdb", MDB_FIXEDMAP, 0664);

	DB_Info info;

	info.env = env;
        info.count = count;
        info.values = values;
	
        rc = pthread_create( &writer, NULL, add_key_values, (void*) &info);
	if (rc)
        {
		fprintf(stderr, "Error creating writer thread\n");
		return 1;
	}

	for (i = 0; i < NUM_READERS; i++)
	{
		rc = pthread_create( &readers[i], NULL, read_key_values, (void*) &info);
		if (rc)
        	{
			fprintf(stderr, "Error creating writer thread\n");
			return 1;
		}
	}

 
        rc = pthread_create( &eraser, NULL, delete_key_values, (void*) &info);

        pthread_join(writer, NULL);

	for (i=0; i < NUM_READERS; i++)
	{
		pthread_join(readers[i], NULL);        
	}
       
	pthread_join(eraser, NULL);
	mdb_env_close(env);

	assert(!gettimeofday(&tv_end, NULL));
	fprintf(stderr, "time elapsed %ld us\n",
		tv_end.tv_usec - tv_start.tv_usec +
       		(tv_end.tv_sec - tv_start.tv_sec) * 1000000);

	free(values);
#ifdef NVM_STATS
   NVM_PrintNumFlushes();
   NVM_PrintStats();
#endif
	return 0;
}
