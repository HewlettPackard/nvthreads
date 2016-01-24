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
#define _XOPEN_SOURCE 700		/* srandom(), random() */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "mdb.h"
#include <sys/time.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#define KB 1024
#define MB (KB*KB)
#define STR_MAX_LEN 32
#define NAME_MAX_LEN 16
#define SURNAME_MAX_LEN (STR_MAX_LEN-NAME_MAX_LEN)
#define RECORD_MAX_LEN (NAME_MAX_LEN+SURNAME_MAX_LEN+STR_MAX_LEN)

#include "emp_data2.h"

#define SIM				"/run/simulator"
#define SIM_SEQ			SIM"/seq"
#define SIM_RND			SIM"/rnd"
#define MIXED			10000
//#define WRITES			 9900  /* READS will be WRITES/MIXED ratio */

void help(void);
void batch(int);
int getListSize(void);

MDB_env *env;
MDB_dbi dbi;
MDB_val key, data;
MDB_txn *txn;
MDB_stat mst;
MDB_cursor *cursor;
int record_num=0;
char type [6];
int WRITES = 10;

int main(int argc,char * argv[])
{
	int rc, i;
	int count;
	char sval[RECORD_MAX_LEN], name[NAME_MAX_LEN], surname[SURNAME_MAX_LEN];
    int emp_id, num_min=0, num_max=0, step=0;
    int choice;
	float la=0., lo=0.;/*, elapsed=0.;*/
    char input_choice[32];
	struct timeval tv_end, tv_start;
	char fpath[32], prompt[16];
	FILE *fd;

	if(argc==2)
		WRITES=atoi(argv[1]);

	if((100*WRITES>=MIXED)||(WRITES<=0)){
		printf("Failed! Invalid percent of writes %d\nPlease inform it in the range [1,99]\n", WRITES);
		exit(1);
	}

	WRITES*=100;
	printf("Starting \"%s\"\n",argv[0]);

#if 0
	if((strcmp(argv[0],"./emp_data_disk")==0)||       /* disk */
		(strcmp(argv[0],"emp_data_disk")==0)) {
		snprintf(type, sizeof(type), "disk");
		snprintf(prompt, sizeof(prompt), "%s", type);
	} else if((strcmp(argv[0],"./emp_data_nvm_optimized")==0)||   /* nvm no flush */
		(strcmp(argv[0],"emp_data_nvm_optimized")==0)) {
		snprintf(type, sizeof(type), "nvm");
		snprintf(prompt, sizeof(prompt), "%s_optimized", type);
	} else if((strcmp(argv[0],"./emp_data_nvm")==0)|| /* nvm with flush */
		(strcmp(argv[0],"emp_data_nvm")==0)) {
		/* OK, this may be a little bit confusing. Just remember that nvm and flush means not optimized. TODO: fix the type to be nvm also, and make the appropriate changes in the filename for backend webservice. */
		snprintf(type, sizeof(type), "flush");
		snprintf(prompt, sizeof(prompt), "nvm"); 
	} else {
		printf("Urrecognized executable name \"%s\". Quitting!\n", argv[0]);
		exit(1);
	}
#endif
    
    snprintf(type, sizeof(type), "flush");
    snprintf(prompt, sizeof(prompt), "nvm"); 

    rc = mdb_env_create(&env);
    rc = mdb_env_set_mapsize(env, 10485760);
    rc = mdb_env_open(env, "./testdb", MDB_FIXEDMAP /*|MDB_NOSYNC*/, 0664);

    rc = mdb_txn_begin(env, NULL, 0, &txn);
    rc = mdb_open(txn, NULL, 0, &dbi);
    rc = mdb_stat(txn, dbi, &mst);
    mdb_txn_abort(txn);

	record_num=getListSize();
	//	printf("Num entries = %d\n", record_num); /*(int)mst.ms_entries); */

    help();
    while (1)
    {
        printf("%s> ", prompt);

        scanf("%s", input_choice);
        if (!strcmp(input_choice, "put"))
        {
            choice = 1;
            scanf("%s %s %f %f", name, surname, &la, &lo);
        }
        else if (!strcmp(input_choice, "get"))
        {
            choice = 2;
            scanf("%d", &emp_id);
        }
        else if (!strcmp(input_choice, "del"))
        {
            choice = 3;
            scanf("%d", &emp_id);
        }
        else if (!strcmp(input_choice, "batch"))
        {
            choice = 4;
            scanf("%d %d %d", &num_min, &num_max, &step);
        }
        else if (!strcmp(input_choice, "list"))
        {
			choice = 5;
        }
        else if (!strcmp(input_choice, "init"))
        {
			choice = 6;
        }
        else if (!strcmp(input_choice, "num"))
        {
			choice = 7;
        }
        else if (!strcmp(input_choice, "quit"))
        {
            break;
        }
        else if (!strcmp(input_choice, "help"))
        {
			help();
            continue;
        }
        else 
        {
            printf("Invalid option \"%s\"\n", input_choice);
			help();
            continue;
        }

        switch(choice) 
        {
            case 1:
                key.mv_size = sizeof(int);
                emp_id = record_num;
                key.mv_data = &emp_id;

                data.mv_size = sizeof(sval);
                data.mv_data = sval;

				snprintf(sval, RECORD_MAX_LEN, "%s %s %.02f, %.02f", name, surname, la, lo);

/*
                printf("About to insert %d -> %.*s\n",
                       *(int*)key.mv_data,
                       (int)data.mv_size, (char*)data.mv_data);
*/
                rc = mdb_txn_begin(env, NULL, 0, &txn);
                rc = mdb_open(txn, NULL, 0, &dbi);
                rc = mdb_put(txn, dbi, &key, &data, 0);
                if (!rc)
                    printf("Inserted %.*s with id %d\n",
                           (int)data.mv_size, (char*)data.mv_data,
                           *(int*)key.mv_data);
                else printf("Insertion failed\n");
                rc = mdb_txn_commit(txn);
                ++record_num;
                break;
            case 2:
                key.mv_size = sizeof(int);
                key.mv_data = &emp_id;
                
                rc = mdb_txn_begin(env, NULL, 1, &txn);
                if ((rc = mdb_get(txn, dbi, &key, &data)) == 0)
                    printf("Returned %.*s with id %d\n",
                           (int)data.mv_size, (char *) data.mv_data,
                           *(int*)key.mv_data);
                else printf("None found\n");
                mdb_txn_commit(txn);
                break;
            case 3:
                key.mv_size = sizeof(int);
                key.mv_data = &emp_id;
                
                rc = mdb_txn_begin(env, NULL, 1, &txn);
                rc = mdb_del(txn, dbi, &key, NULL);
                if (rc)
                {
                    printf("Deletion failed\n");
                    mdb_txn_abort(txn);
                }
                else
                {
                    printf("Deleted id %d\n", *(int*)key.mv_data);
                    mdb_txn_commit(txn);
					record_num--;
                }
                break;
            case 4:
				printf("Running batch mode:\nstart:%d\nstep:%d\nend:%d\n", num_min, step, num_max);
				for (i=num_min; i<num_max; i+=step){
					printf("Running for size=%d\n", i);
#if 0                    
					snprintf(fpath, 32, "%s/size", SIM);
					fd = fopen(fpath, "w+");
					if(!fd){
						printf("Error opening file %s: %s\n", fpath, strerror(errno));
						exit(1);
					}
					fprintf(fd, "%d", i);
					fflush(fd);
					fclose(fd);
#endif
					assert(!gettimeofday(&tv_start, NULL));		
					batch(i);
					assert(!gettimeofday(&tv_end, NULL));
/*
					elapsed=((float)(tv_end.tv_usec - tv_start.tv_usec)/1000000 + (float)(tv_end.tv_sec - tv_start.tv_sec));
					if(elapsed<1.);
						sleep(1);
*/
				}
                break;
			case 5:
				printf("Listing entries... \n");

				rc = mdb_env_stat(env, &mst);
				rc = mdb_txn_begin(env, NULL, 1, &txn);
				rc = mdb_cursor_open(txn, dbi, &cursor);

				while ((rc = mdb_cursor_get(cursor, &key, &data, MDB_NEXT)) == MDB_SUCCESS){
					printf("Returned %.*s with id %d\n",
						(int)data.mv_size, (char *) data.mv_data,
						*(int*)key.mv_data);
				}
				mdb_txn_commit(txn);

				printf("End of list.\n");
				break;
			case 6:
				printf("Initializing database...\n");
                for (count = 0; count < 100; ++count)
                {
                    key.mv_size = sizeof(int);
                	emp_id = record_num;
                    key.mv_data = &emp_id;

                    data.mv_size = STR_MAX_LEN;
                    data.mv_data = sval;
					snprintf(sval, RECORD_MAX_LEN, "%s %s", data0[count].name, data0[count].geo);

                    rc = mdb_txn_begin(env, NULL, 0, &txn);
                    rc = mdb_open(txn, NULL, 0, &dbi);
                    rc = mdb_put(txn, dbi, &key, &data, MDB_NOOVERWRITE);
                	if (!rc) {
						printf("Inserted %.*s with id %d\n",
								(int)data.mv_size, (char*)data.mv_data,
								*(int*)key.mv_data);
						rc = mdb_txn_commit(txn);
						memset(sval, 0, STR_MAX_LEN);
						++record_num;
					} else {
						printf("Insertion failed: %s\n", mdb_strerror(rc));
			    		mdb_txn_abort(txn);
					}
                }
				
				break;
			case 7:
				printf("Getting number of entries...\n");
			    printf("Num entries = %d\n", record_num);
				break;
            default:
                assert(0);
                break;
        }
		record_num = getListSize(); /*mst.ms_entries;*/
    }

	mdb_close(env, dbi);
	mdb_env_close(env);

	return 0;
}

void batch(int count){
	char sval[RECORD_MAX_LEN] = {0};
	int rc, rnd, i, idx;
	struct timespec te, ts;
	float len=0., data_rate=0., elapsed=0.;
	char fpath[32] = {0};
	FILE *fd;
	float wr_cnt=0., rd_cnt=0.;

	srandom(time(NULL));

	/* Start with random (rnd=1) */
	for (rnd=1; rnd>=0; rnd--){

		printf("===========================================================\n");
		printf("Writing %s data...\n", rnd ? "random" : "sequential");

		len=0.;
		elapsed=0.;
		data_rate=0.;
		for (i=0;i<count;i++) {
			key.mv_size = sizeof(int);
			idx = rnd ? random()%record_num : i%record_num;
			key.mv_data = &idx;
			data.mv_size = sizeof(sval);
			data.mv_data = sval;
			snprintf(sval, sizeof(sval), "%s %s", data1[idx%(sizeof(data1)/(sizeof(data1[0])))].name, 
				data1[idx%(sizeof(data1)/(sizeof(data1[0])))].geo);

			txn=NULL;
			assert(!clock_gettime(CLOCK_REALTIME, &ts));
			rc = mdb_txn_begin(env, NULL, 0, &txn);
			//			rc = mdb_open(txn, NULL, 0, &dbi);
			rc = mdb_put(txn, dbi, &key, &data, 0);

			if (rc) {
				printf("Failed to write value: \"%s\" for key [%d] (%s)\nAborting!\n", sval, idx, mdb_strerror(rc));
				mdb_txn_abort(txn);
				abort();
			} else {
			/*	printf("Wrote key %d, with data: %s\n", idx, sval); */
				rc = mdb_txn_commit(txn);
				assert(!clock_gettime(CLOCK_REALTIME, &te));
				elapsed += ((float)(te.tv_nsec - ts.tv_nsec)/1000000000. + (float)(te.tv_sec - ts.tv_sec));
				len=strnlen(sval, sizeof(sval));
				data_rate += len;
			}
		}

		data_rate = data_rate/elapsed/count;
/*		printf("Write data_rate for %s: %.01f KB/second\n", rnd ? "random" : "sequential", data_rate);*/
#if 0
		snprintf(fpath, sizeof(fpath), "%s/write_%s", rnd ? SIM_RND : SIM_SEQ, type);
		fd = fopen(fpath, "w+");
		if(!fd){
			printf("Error opening file %s: %s\n", fpath, strerror(errno));
			exit(1);
		}
		fprintf(fd, "%.0f", data_rate);
		fflush(fd);
		fclose(fd);

		system("/bin/sync");
#endif
        
		printf("===========================================================\n");
		printf("Mixed %s data...\n", rnd ? "random" : "sequential");

		record_num=getListSize();

		len=0.;
		elapsed=0.;
		data_rate=0.;
		rd_cnt=0.;
		wr_cnt=0.;
		for (i=0;i<count;i++) {
			key.mv_size = sizeof(idx);
			idx = rnd ? random()%record_num : i%record_num;
			key.mv_data = &idx;
			data.mv_size = sizeof(sval);
			data.mv_data = sval;
			snprintf(sval, sizeof(sval), "%s %s", data2[idx%(sizeof(data2)/(sizeof(data2[0])))].name, 
				data2[idx%(sizeof(data2)/(sizeof(data2[0])))].geo);

			txn=NULL;
			assert(!clock_gettime(CLOCK_REALTIME, &ts));
			rc = mdb_txn_begin(env, NULL, 0, &txn);
			//			rc = mdb_open(txn, NULL, 0, &dbi);
			/* Selecting if we'll read or write */
			if(random()%MIXED < WRITES) { 	/* Now we write... */
				wr_cnt+=1.;
				rc = mdb_put(txn, dbi, &key, &data, 0);
			} else { 						/* ...and now we read! */
				rd_cnt+=1.;
				rc = mdb_get(txn, dbi, &key, &data);
			}
			if (rc) {
				if(MDB_NOTFOUND != rc) {
					/*printf("Failed to read/write key: %d (%s)\nAborting!\n", idx, mdb_strerror(rc));*/
					mdb_txn_abort(txn); 
					/* We may have some deleted item in db. Don't need to abort on that. Just keep going...
					abort();*/
				}
			} else {
				rc = mdb_txn_commit(txn);
				assert(!clock_gettime(CLOCK_REALTIME, &te));

				elapsed+=((float)(te.tv_nsec - ts.tv_nsec)/1000000000. + (float)(te.tv_sec - ts.tv_sec));
				len=strnlen(sval, sizeof(sval));
				data_rate += len;
/*				printf("Read key %d, data %.*s, size %.01f, time %.09f, datarate %.03f, mean %.03f\n", idx, 
						(int) data.mv_size, (char *) data.mv_data, len, elapsed, data_rate, i==0?data_rate:data_rate/i);*/
			}
		}
		data_rate=data_rate/elapsed/count/KB;

/*		printf("reads:%.0f\twrites:%.0f\n", 100./(1. + wr_cnt/rd_cnt), 100./(1. + rd_cnt/wr_cnt));*/

/*		printf("%.01f bytes in %.06f seconds: %.01f byte/second\n", len, elapsed, data_rate);*/
/*		printf("Read data_rate for %s: %.01f MB/second\n", rnd ? "random" : "sequential", data_rate);*/
#if 0
		snprintf(fpath, sizeof(fpath), "%s/read_%s", rnd ? SIM_RND : SIM_SEQ, type);
		fd = fopen(fpath, "w+");
		if(!fd){
			printf("Error opening file %s: %s\n", fpath, strerror(errno));
			exit(1);
		}
		fprintf(fd, "%.0f", data_rate);
		fflush(fd);
		fclose(fd);
#endif    
	}
}

int getListSize(void){
	int rc, count=0;

	rc = mdb_env_stat(env, &mst);
	rc = mdb_txn_begin(env, NULL, 1, &txn);
	rc = mdb_cursor_open(txn, dbi, &cursor);

	while ((rc = mdb_cursor_get(cursor, &key, &data, MDB_NEXT)) == MDB_SUCCESS)
		count++;

	mdb_txn_commit(txn);
	return count;
}

void help(void){

    printf("Options: put <name>, get <id>, del <id>, batch <min> <max> <step>, num, list, init, help, quit.\n");

}

