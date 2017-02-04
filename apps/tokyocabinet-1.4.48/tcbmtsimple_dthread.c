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
TCBDB* bdb;

pthread_mutex_t lock;
struct thread_args {
  int thread_id;
//TCBDB* bdb;
};

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
//TCBDB* bdb;
  int ecode;

  /* create the object */
  bdb = tcbdbnew();
  /* set mutual exclusion for threading*/
  if (!tcbdbsetmutex(bdb)) {
    ecode = tcbdbecode(bdb);
    fprintf(stderr, "setmutex error: %s\n", tcbdberrmsg(ecode));
  }

  /* open the database */
  if (!tcbdbopen(bdb, DATABASE, BDBOWRITER|BDBOCREAT)) {
    ecode = tcbdbecode(bdb);
    fprintf(stderr, "open error: %s\n", tcbdberrmsg(ecode));
    exit(-1);
  }
//traverse_btree(0);
  return bdb;
}

bool query_record(int thread_id, int key_index, int written, int base) {
  int query_index = (key_index % written) + base;
//int query_index = base; // always query the first thread-local record
  int ecode;
  char* key = (char*)malloc(RECBUFSIZE);
  char* value;
  int keylen;
  keylen = sprintf(key, "%08d", query_index);
  memset(&key[keylen], 'a', RECBUFSIZE - keylen);
  key[RECBUFSIZE - 1] = '\0';

//  printf("%d: thread %d querying record with key: %d\n", key_index, thread_id, query_index);
//  printf("key (%zu bytes):\t%s\n", sizeof(key), key);
  pthread_mutex_lock(&lock);
  value = tcbdbget2(bdb, key);
  if (!value) {
    pthread_mutex_unlock(&lock);
    int retry = 0;
    while (!value) {
      pthread_mutex_lock(&lock);
//    /* Try the next query */
//    query_index++;
//    keylen = sprintf(key, "%08d", query_index);
//    memset(&key[keylen], 'a', RECBUFSIZE - keylen);
//    key[RECBUFSIZE - 1] = '\0';
//
//    /* Issue query again */
      value = tcbdbget2(bdb, key);
      ecode = tcbdbecode(bdb);
      fprintf(stderr, "Error %s.\n", tcbdberrmsg(ecode));
      fprintf(stderr, "%d: thread %d couldn't query record with key index: %d\n", key_index, thread_id, query_index);
      fprintf(stderr, "key: %s\n", key);
      fprintf(stderr, "value: %s\n", value);
      pthread_mutex_unlock(&lock);

      retry++;
      if (retry > 10) {
        fprintf(stderr, "thread %d tried 10 times, abort.\n", thread_id);
//      traverse_btree(thread_id);
        return false;
      }
    }
    free(value);
//  return false;
  } else {
//  printf("value (%d bytes):\t%s\n", valuelen, value);
    free(value);
    pthread_mutex_unlock(&lock);
  }

  return true;
}


/* thread read workload */
void thread_read_worker(void* args) {
  struct thread_args* targs = (struct thread_args*)args;
//TCBDB* bdb = targs->bdb;
  int thread_id = targs->thread_id;
  int records;
  int base = thread_id * NRECORDS_PER_THREAD;
  int updatetotal = (NRECORDS_PER_THREAD * WRITE_PERCENT) / 100;
  int readtotal = NRECORDS_PER_THREAD - updatetotal;

  bool result;
  for (records = 0; records < readtotal; records++) {
    /* query record */
    result = query_record(thread_id, base + records, updatetotal, base);
    if (!result) {
      fprintf(stderr, "thread %d stop execution\n", thread_id);
//    return NULL;
      return;
    }
  }
  printf("thread %d read %d records\n", thread_id, readtotal);
//return NULL;
}



bool update_record(int thread_id, int key_index) {
  char* key = (char*)malloc(RECBUFSIZE);
  char* value = (char*)malloc(RECBUFSIZE);
  int keylen;
  int retry;

  /* generate a record */
  keylen = sprintf(key, "%08d", key_index);
  memset(&key[keylen], 'a', RECBUFSIZE - keylen);
  key[RECBUFSIZE - 1] = '\0';
  memset(value, 'x', RECBUFSIZE);
  value[RECBUFSIZE - 1] = '\0';

  retry = 0;
  /* insert a record */
  pthread_mutex_lock(&lock);
//while (tcbdbput(bdb, key, RECBUFSIZE, value, RECBUFSIZE) == false) {
  while (tcbdbput2(bdb, key, value) == false) {
    int ecode = tcbdbecode(bdb);
    fprintf(stderr, "put error: %s\n", tcbdberrmsg(ecode));
    printf("retry key:\t%s\n", key);
    printf("retry value:\t%s\n", value);
    retry++;
    if (retry > 0) {
      fprintf(stderr, "failed to put %d times, stop\n", retry);
      pthread_mutex_unlock(&lock); 
//    traverse_btree(thread_id);
      return false;
    }
  }

//printf("%d: thread %d updated key (%d bytes) / value (%d bytes):\n", key_index, thread_id, RECBUFSIZE, RECBUFSIZE);
//printf("key:\t%s\n", key);
//printf("value:\t%s\n", value);

  pthread_mutex_unlock(&lock);
  return true;
}

/* thread write workload */
void* thread_update_worker(void* args) {
  struct thread_args* targs = (struct thread_args*)args;
//TCBDB* bdb = targs->bdb;
  int thread_id = targs->thread_id;
  int records;
  int base = thread_id * NRECORDS_PER_THREAD;
  int updatetotal = (NRECORDS_PER_THREAD * WRITE_PERCENT) / 100;

//if (thread_id == 0) {
//traverse_btree(thread_id);
//}
//printf("thread %d updating %d (out of %d) records, write percentage: %d%%\n", thread_id, updatetotal, NRECORDS_PER_THREAD, WRITE_PERCENT);
  for (records = 0; records < updatetotal; records++) {
    /* update a record */
    int key_index = base + records;
    if (update_record(thread_id, key_index) == false){
      return NULL;
    }
  }
//printf("thread %d updated %d records, #records in bdb: %lu\n", thread_id, updatetotal, tcbdbrnum(bdb));
//if (thread_id == 0) {
//  traverse_btree(thread_id);
//}
  printf("thread %d finished updating, starting read workload\n", thread_id);
  thread_read_worker(args);
  return NULL;
}


int main(int argc, char** argv) {
//TCBDB* bdb;
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

  /* Open database */
  bdb = openTCBDB(DATABASE);

//traverse_btree(0);

  /* create updater threads */
  printf("starting write workload\n");
  pthread_t tid[NTHREADS];
  struct thread_args targs[NTHREADS];
  pthread_mutex_init(&lock, NULL);
  for (i = 0; i < NTHREADS; i++) {
    targs[i].thread_id = i;
//  targs[i].bdb = bdb;
    pthread_create(&tid[i], NULL, thread_update_worker, &targs[i]);
  }
  for (i = 0; i < NTHREADS; i++) {
    pthread_join(tid[i], NULL);
  }


  /* traverse records */
//traverse_btree(0);

  /* close the database */
  printf("closing databse\n");
  if (!tcbdbclose(bdb)) {
    int ecode = tcbdbecode(bdb);
    fprintf(stderr, "close error: %s\n", tcbdberrmsg(ecode));
  }
  /* delete the object */
//printf("deleting databse");
//tcbdbdel(bdb);

  gettimeofday(&end, NULL);
  elapsed = 1000 * (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000;

  printf("%llu ms\n", elapsed);
  return 0;
}
