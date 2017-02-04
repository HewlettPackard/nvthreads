#include <tcutil.h>
#include <tcbdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>
#include <sys/time.h>
#include <limits.h>
#include <string.h>

char DATABASE[128];
int NTHREADS;
int NRECORDS;
int NRECORDS_PER_THREAD;
int NQUERIES;
int NQUERIES_PER_THREAD;
int RECBUFSIZE;
int WRITE_PERCENT;
int DELAY;
int DELAY_THRESHOLD; 

TCBDB* bdb;

pthread_mutex_t lock;
struct thread_args {
  int thread_id;
};


#define nvmfs_cpu_freq_mhz 3600
#define NS2CYCLE(__ns) ((__ns) * nvmfs_cpu_freq_mhz / 1000LLU)

static inline unsigned long long asm_rdtsc(void) {
  unsigned hi, lo;
  __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
  return((unsigned long long)lo) | (((unsigned long long)hi) << 32 );
}

void emulate_latency_ns(long long ns) {
  unsigned long long cycles;
  unsigned long long start;
  unsigned long long stop;

  start = asm_rdtsc();
  cycles = NS2CYCLE(ns);

  do {
    /* RDTSC doesn't necessarily wait for previous instructions to complete so 
     * a serializing instruction is usually used to ensure previous
     * instructions have completed. However, in our case this is a desirable
     * property since we want to overlap the latency we emulate with the
     * actual latency of the emulated instruction.
     */
    stop = asm_rdtsc();
  } while (stop - start < cycles);
}

/* list out all records in key order */
void traverse_btree(int thread_id) {
  BDBCUR* cur;
  char* key, *value;
  int nrecord = 0;
  pthread_mutex_lock(&lock);
  cur = tcbdbcurnew(bdb);
  tcbdbcurfirst(cur);
  printf("---thread %d traversing %lu records---\n", thread_id, tcbdbrnum(bdb));
  while ((key = tcbdbcurkey2(cur)) != NULL) {
    value = tcbdbcurval2(cur);
    if (value) {
      printf("%d: key: %s\n", nrecord, key);
      printf("%d: value: %s\n", nrecord, value);
      free(value);
      nrecord++;
    }
    free(key);

    tcbdbcurnext(cur);
  }
  printf("------------------------\n");
  tcbdbcurdel(cur);
  pthread_mutex_unlock(&lock);
}


/* open a database and return db object */
TCBDB* openTCBDB(char* filename) {
  int ecode;
  pthread_mutex_lock(&lock);
  /* create the object */
  bdb = tcbdbnew();
  /* set mutual exclusion for threading*/
//if (!tcbdbsetmutex(bdb)) {
//  ecode = tcbdbecode(bdb);
//  fprintf(stderr, "setmutex error: %s\n", tcbdberrmsg(ecode));
//}

  /* open the database */
  if (!tcbdbopen(bdb, DATABASE, BDBOWRITER)) {
    ecode = tcbdbecode(bdb);
    fprintf(stderr, "open error: %s\n", tcbdberrmsg(ecode));
    exit(-1);
  }
  pthread_mutex_unlock(&lock);
//traverse_btree(0);
  return bdb;
}

void closeTCBDB(void) {
  pthread_mutex_lock(&lock);
  printf("closing databse\n");
  if (!tcbdbclose(bdb)) {
    int ecode = tcbdbecode(bdb);
    fprintf(stderr, "close error: %s\n", tcbdberrmsg(ecode));
  }
  /* delete the object */
  printf("deleting databse\n");
  tcbdbdel(bdb);
  pthread_mutex_unlock(&lock);
}

bool query_record_by_index(int thread_id, int query_index) {
  int ecode;
  char key[RECBUFSIZE];
  char* value;
  int keylen, valuelen;
  keylen = sprintf(key, "%08d", query_index);
  memset(&key[keylen], 'a', RECBUFSIZE - keylen);
  key[RECBUFSIZE - 1] = '\0';

  value = tcbdbget(bdb, key, RECBUFSIZE, &valuelen);
  if (!value) {
    ecode = tcbdbecode(bdb);
    fprintf(stderr, "Error %s.\n", tcbdberrmsg(ecode));
    fprintf(stderr, "thread %d couldn't query record with key index: %d\n", thread_id, query_index);
    fprintf(stderr, "key: %s\n", key);
    fprintf(stderr, "value: %s\n", value);
    return false;
  } else {
    printf("thread %d query:\n", thread_id);
    printf("key: %s\n", key);
    printf("value: %s\n", value);
    free(value);
  }
  return true;
}

bool query_all_records(int thread_id) {
  int i;
  for (i = 0; i < NRECORDS; i++) {
    bool result;
    pthread_mutex_lock(&lock);
    result = query_record_by_index(thread_id, i);
    pthread_mutex_unlock(&lock);
    if (!result) {
      printf("failed to query all records, thread %d stop execution\n", thread_id);
      return false;
    }
  }
  return true;
}

bool query_record(int thread_id, int key_index, int written, int base) {
//int query_index = (key_index % written) + base;
  int query_index = key_index;
  int ecode;
  char key[RECBUFSIZE];
  char* value;
  int keylen, valuelen;
  keylen = sprintf(key, "%08d", query_index);
  memset(&key[keylen], 'a', RECBUFSIZE - keylen);
  key[RECBUFSIZE - 1] = '\0';

  pthread_mutex_lock(&lock);
  value = tcbdbget(bdb, key, RECBUFSIZE, &valuelen);
  if (!value) {
    ecode = tcbdbecode(bdb);
    fprintf(stderr, "Error %s.\n", tcbdberrmsg(ecode));
    fprintf(stderr, "%d: thread %d couldn't query record with key index: %d\n", key_index, thread_id, query_index);
    fprintf(stderr, "key: %s\n", key);
    fprintf(stderr, "value: %s\n", value);
    pthread_mutex_unlock(&lock);
    return false;
  } else {
//  printf("thread %d query index: %d\n", thread_id, query_index);
//  printf("key:\t%s\n", key);
//  printf("value:\t%s\n", value);
    free(value);
    pthread_mutex_unlock(&lock);
  }
  return true;
}


/* thread read workload */
void thread_read_worker(void* args) {
  struct thread_args* targs = (struct thread_args*)args;
  int thread_id = targs->thread_id;
  int records;
  int base = thread_id * NRECORDS_PER_THREAD;
  int updatetotal = (NQUERIES_PER_THREAD * WRITE_PERCENT) / 100;
  int readtotal = NQUERIES_PER_THREAD - updatetotal;

  bool result;
  for (records = 0; records < readtotal; records++) {
    /* query record */
    int key_index = base + (records % NRECORDS_PER_THREAD);
//  printf("thread %d reading key_index: %d\n", thread_id, key_index);
    result = query_record(thread_id, key_index, updatetotal, base);
    if (!result) {
      fprintf(stderr, "thread %d stop execution\n", thread_id);
      return;
    }
  }
  printf("thread %d read %d records\n", thread_id, readtotal);
}



bool update_record(int thread_id, int key_index, unsigned long long *delay) {
  char key[RECBUFSIZE];
  char value[RECBUFSIZE];
  int keylen;
  int trycount = 0;

//#ifdef PLIB
  __asm__ __volatile__("T1:");
//#endif

  memset(value, 'x', RECBUFSIZE);
  value[RECBUFSIZE - 1] = '\0';

 retry:
  /* generate a record */
  keylen = sprintf(key, "%08d", key_index);
//keylen = 0;
  memset(&key[keylen], 'a', RECBUFSIZE - keylen);
  key[RECBUFSIZE - 1] = '\0';

  /* update a record */
  pthread_mutex_lock(&lock);

//#ifdef PLIB
  tcbdbtranbegin(bdb);
//#endif

  if (tcbdbput(bdb, key, RECBUFSIZE, value, RECBUFSIZE) == false) {
    int ecode = tcbdbecode(bdb);
    fprintf(stderr, "put error: %s\n", tcbdberrmsg(ecode));
    printf("%d: thread %d failed to update key (%d bytes) / value (%d bytes):\n", key_index, thread_id, RECBUFSIZE, RECBUFSIZE);
    printf("key:\t%s\n", key);
    printf("value:\t%s\n", value);
    pthread_mutex_unlock(&lock);

//#ifdef PLIB
    tcbdbtranabort(bdb);
//#endif
    key_index++;
    trycount++;
    if (trycount > 3) {
      traverse_btree(thread_id);
      return false;
    }
    goto retry;
  }
//printf("%d: thread %d updated key (%d bytes) / value (%d bytes):\n", key_index, thread_id, RECBUFSIZE, RECBUFSIZE);
//printf("key:\t%s\n", key);
//printf("value:\t%s\n", value);

//#ifdef PLIB
  tcbdbtrancommit(bdb);

  /* nvmfs delays */
  *delay = *delay + DELAY;
  if ( *delay >= DELAY_THRESHOLD) {
    emulate_latency_ns(*delay);
    *delay = 0;
  }
//#endif

  pthread_mutex_unlock(&lock);


//#ifdef PLIB
  __asm__ __volatile__("T2:");
//#endif
  return true;
}

/* thread write workload */
void* thread_update_worker(void* args) {
  struct thread_args* targs = (struct thread_args*)args;
  int thread_id = targs->thread_id;
  int records;
  int base = thread_id * NRECORDS_PER_THREAD;
  int updatetotal = (NQUERIES_PER_THREAD * WRITE_PERCENT) / 100;
  unsigned long long delay = 0;

//if (!query_all_records(thread_id)) {
//  printf("thread %d failed to query all records, stop execution\n", thread_id);
//  return NULL;
//}

//printf("thread %d updating %d (out of %d) records, write percentage: %d%%\n", thread_id, updatetotal, NQUERIES_PER_THREAD, WRITE_PERCENT);
  for (records = 0; records < updatetotal; records++) {
    /* update a record */
    int key_index = base + (records % NRECORDS_PER_THREAD);
//  printf("thread %d updating key_index: %d\n", thread_id, key_index);
    if (update_record(thread_id, key_index, &delay) == false) {
      return NULL;
    }
  }

  if (delay != 0) {
    emulate_latency_ns(DELAY_THRESHOLD);
  }

  printf("thread %d finished updating %d records, starting read workload\n", thread_id, updatetotal);
  thread_read_worker(args);
  return NULL;
}


void insert_record(int key_index, int keysize) {
  int ecode;
  char key[RECBUFSIZE];
  char value[RECBUFSIZE];
  int keylen;
  bool result;

  /* generate a record */
  keylen = sprintf(key, "%08d", key_index);
  memset(&key[keylen], 'a', keysize - keylen);
  key[keysize - 1] = '\0';
  memset(value, 'b', keysize);
  value[keysize - 1] = '\0';

  /* insert a record */
  result = tcbdbput(bdb, key, keysize, value, keysize);
  if (!result) {
    ecode = tcbdbecode(bdb);
    fprintf(stderr, "put error: %s\n", tcbdberrmsg(ecode));
  } else {
//  printf("%d: inserted key (%d bytes) / value (%d bytes):\n", key_index, keysize, keysize);
//  printf("key:\t%s\n", key);
//  printf("value:\t%s\n", value);
  }
}

bool create_database(char* filename, int nrecords, int keysize) {
  int ecode;
  int i;
  int ret;

  /* remove existing file first */
  ret = remove(filename);
  if (ret == 0) {
    printf("deleted existing file %s.\n", filename);
  } else {
    printf("database %s does not exist, creating it now.\n", filename);
  }

  /* create the object */
  bdb = tcbdbnew();

  /* open the database */
//#ifdef PLIB
  /* set mutual exclusion for threading*/
  if (!tcbdbsetmutex(bdb)) {
    ecode = tcbdbecode(bdb);
    fprintf(stderr, "setmutex error: %s\n", tcbdberrmsg(ecode));
  }

  if (!tcbdbopen(bdb, filename, BDBOWRITER | BDBOCREAT | BDBOTSYNC)) {
//#else
//if (!tcbdbopen(bdb, filename, BDBOWRITER | BDBOCREAT)) {
//#endif
    ecode = tcbdbecode(bdb);
    fprintf(stderr, "open error: %s\n", tcbdberrmsg(ecode));
    return false;
  }

  /* generate records */
  for (i = 0; i < nrecords; i++) {
    insert_record(i, keysize);
  }
  printf("database with %d created at: %s\n", nrecords, filename);
  return true;
}

int main(int argc, char** argv) {
  int i, result;
  struct timeval start, end;
  unsigned long long elapsed;

//#ifdef PLIB
  printf("Pthreads version, will call tcbdbtranbegin and tcbdbtrancommit to persist data\n");
//#endif

  /* check input */
  if (argc < 7) {
    fprintf(stderr, "Usage: %s database nthreads nqueries recordsize writepercent delay\n", argv[0]);
    exit(-1);
  }
  strcpy(DATABASE, argv[1]);
  NTHREADS = atoi(argv[2]);
  NQUERIES = atoi(argv[3]);
  RECBUFSIZE = atoi(argv[4]);
  WRITE_PERCENT = atoi(argv[5]);
//DELAY = atoi(argv[6]);
  DELAY = 0;
//DELAY = RECBUFSIZE / 4; // 2 ns delay per 8 bytes
//DELAY = RECBUFSIZE * 2; // 16 ns delay per 8 bytes
  DELAY = DELAY * 3; // key and value and metadata
  DELAY_THRESHOLD = 50000; // batch delay 50000ns
  NQUERIES_PER_THREAD = NQUERIES / NTHREADS;
  NRECORDS = 1000;
  NRECORDS_PER_THREAD = NRECORDS / NTHREADS;
  printf("%s %d threads, %d queries, %d bytes/record, %d queries/thread, %d writepercent, %d ns delay/xact\n",
         argv[0], NTHREADS, NQUERIES, RECBUFSIZE, NQUERIES_PER_THREAD, WRITE_PERCENT, DELAY);

  srand(time(NULL));

  /* Create database */
  gettimeofday(&start, NULL);
  result = create_database(DATABASE, NRECORDS, RECBUFSIZE);
  if (!result) {
    fprintf(stderr, "error: failed to create database\n");
    exit(-1);
  }
  gettimeofday(&end, NULL);
  elapsed = 1000 * (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000;
  printf("populating time %llu ms\n", elapsed);

  /* Verify database content */
//traverse_btree(0);

  gettimeofday(&start, NULL);

  /* create worker threads */
  printf("starting write workload\n");
  pthread_t tid[NTHREADS];
  struct thread_args targs[NTHREADS];
  pthread_mutex_init(&lock, NULL);
  for (i = 0; i < NTHREADS; i++) {
    targs[i].thread_id = i;
    pthread_create(&tid[i], NULL, thread_update_worker, &targs[i]);
  }
  for (i = 0; i < NTHREADS; i++) {
    pthread_join(tid[i], NULL);
  }


  /* traverse records */
//traverse_btree(0);

  /* close the database */
  closeTCBDB();

  gettimeofday(&end, NULL);
  elapsed = 1000 * (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000;

  printf("query time %llu ms\n", elapsed);
  return 0;
}
