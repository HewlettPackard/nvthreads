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
int RECBUFSIZE;
int WRITE_PERCENT;

pthread_rwlock_t lock_rw = PTHREAD_RWLOCK_INITIALIZER;

pthread_mutex_t lock;
struct thread_args {
  int thread_id;
  TCBDB* bdb;
};

/* list out all records in key order */
void traverse_btree(void* args) {
  struct thread_args* targs = (struct thread_args*)args;
  TCBDB* bdb = targs->bdb;
  int thread_id = targs->thread_id;
  BDBCUR* cur;
  char* key, *value;
  pthread_mutex_lock(&lock);
  cur = tcbdbcurnew(bdb);
  tcbdbcurfirst(cur);
  printf("---thread %d: traversing records---\n", thread_id);
  while ((key = tcbdbcurkey2(cur)) != NULL) {
    value = tcbdbcurval2(cur);
    if (value) {
      printf("key: %s / value: %s\n", key, value);
      free(value);
    }
    free(key);
    tcbdbcurnext(cur);
  }
  tcbdbcurdel(cur);
  pthread_mutex_unlock(&lock);
}

void insert_record(TCBDB* bdb, int thread_id, int key_index) {
  int ecode;
  char key[RECBUFSIZE];
  char value[RECBUFSIZE];

  /* generate a record */
  int keylen = sprintf(key, "%08d", key_index);
  int valuelen = sprintf(value, "%08d", rand() % (NRECORDS * NRECORDS));
  /* insert a record */
  pthread_mutex_lock(&lock);
  if (!tcbdbput(bdb, key, keylen, value, valuelen)) {
    ecode = tcbdbecode(bdb);
    fprintf(stderr, "put error: %s\n", tcbdberrmsg(ecode));
  } else {
//  printf("%d: thread %d put key/value: %s/%s\n", key_index, thread_id, key, value);
  }
  pthread_mutex_unlock(&lock);
}

void query_record(TCBDB* bdb, int thread_id, int key_index, int written, int base) {
  int query_index = key_index % written + base;
  int ecode;
  char key[RECBUFSIZE];
  char *value;
  sprintf(key, "%08d", query_index);
  value = tcbdbget2(bdb, key);
  if (!value) {
    ecode = tcbdbecode(bdb);
    fprintf(stderr, "Error %s. %d: thread %d couldn't query record with key: %d\n", tcbdberrmsg(ecode), key_index, thread_id, query_index);    
  }
//printf("%d: thread %d querying record with key: %d, value: %s\n", key_index, thread_id, query_index, value);
}

/* thread workload */
void* thread_worker(void* args) {
  struct thread_args* targs = (struct thread_args*)args;
  TCBDB* bdb = targs->bdb;
  int thread_id = targs->thread_id;
  int records;
  int base = thread_id * NRECORDS_PER_THREAD;
  int writetotal = (NRECORDS_PER_THREAD * WRITE_PERCENT) / 100;
  int written = 0;
  int start_reading = 0;
//printf("thread %d inserting %d (out of %d) records, write percentage: %d%%\n", thread_id, writetotal, NRECORDS_PER_THREAD, WRITE_PERCENT);
  for (records = 0; records < NRECORDS_PER_THREAD; records++) {
    /* insert record */
    if (written < writetotal) {
      insert_record(bdb, thread_id, base + records);
      written++;
    }
    /* query record */
    else {
      if (!start_reading) {
        start_reading = 1;
//      printf("thread %d starts reading\n", thread_id);
      }
      query_record(bdb, thread_id, base + records, written, base);
    }
  }
  return NULL;
}

int main(int argc, char** argv) {
  TCBDB* bdb;
  int ecode;
  int i;
  struct timeval start, end;
  unsigned long long elapsed;

  /* check input */
  if (argc < 6) {
    fprintf(stderr, "Usage: %s database nthreads nrecords recordsize writepercent\n", argv[0]);
    exit(-1);
  }
  strcpy(DATABASE, argv[1]);
  NTHREADS = atoi(argv[2]);
  NRECORDS = atoi(argv[3]);
  RECBUFSIZE = atoi(argv[4]);
  WRITE_PERCENT = atoi(argv[5]);
  NRECORDS_PER_THREAD = NRECORDS / NTHREADS;
  printf("%s %d threads, %d records, %d bytes/record, %d records/thread, %d writepercent\n",
         argv[0], NTHREADS, NRECORDS, RECBUFSIZE, NRECORDS_PER_THREAD, WRITE_PERCENT);

  srand(time(NULL));
  gettimeofday(&start, NULL);

  /* create the object */
  bdb = tcbdbnew();
  /* set mutual exclusion for threading*/
//if (!tcbdbsetmutex(bdb)) {
//  ecode = tcbdbecode(bdb);
//  fprintf(stderr, "setmutex error: %s\n", tcbdberrmsg(ecode));
//}
  /* open the database */
  if (!tcbdbopen(bdb, DATABASE, HDBOWRITER | HDBOCREAT)) {
    ecode = tcbdbecode(bdb);
    fprintf(stderr, "open error: %s\n", tcbdberrmsg(ecode));
  }
  /* create threads */
  pthread_t tid[NTHREADS];
  struct thread_args targs[NTHREADS];
  pthread_mutex_init(&lock, NULL);
  for (i = 0; i < NTHREADS; i++) {
    targs[i].thread_id = i;
    targs[i].bdb = bdb;
    pthread_create(&tid[i], NULL, thread_worker, &targs[i]);
  }
  for (i = 0; i < NTHREADS; i++) {
    pthread_join(tid[i], NULL);
  }

  /* traverse records */
//traverse_btree(&targs[0]);

  /* close the database */
//printf("closing databse");
//if (!tcbdbclose(bdb)) {
//  ecode = tcbdbecode(bdb);
//  fprintf(stderr, "close error: %s\n", tcbdberrmsg(ecode));
//}
  /* delete the object */
//printf("deleting databse");
//tcbdbdel(bdb);

  gettimeofday(&end, NULL);
  elapsed = 1000 * (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000;

  printf("%llu ms\n", elapsed);
  return 0;
}
