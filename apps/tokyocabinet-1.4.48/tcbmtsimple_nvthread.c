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

pthread_mutex_t lock;
struct thread_args {
  int thread_id;
  TCBDB* bdb;
};

/* list out all records in key order */
void traverse_btree(TCBDB* bdb, int thread_id) {
  BDBCUR* cur;
  char* key, *value;
  pthread_mutex_lock(&lock);
  cur = tcbdbcurnew(bdb);
  tcbdbcurfirst(cur);
  printf("---thread %d traversing %lu records---\n", thread_id, tcbdbrnum(bdb));
  while ((key = tcbdbcurkey2(cur)) != NULL) {
    value = tcbdbcurval2(cur);
    if (value) {
      printf("key: %s\n", key);
      printf("value: %s\n", value);
      free(value);
    }
    free(key);
    tcbdbcurnext(cur);
  }
  printf("------------------------\n");
  tcbdbcurdel(cur);
  pthread_mutex_unlock(&lock);
}

#if 0
bool query_record(TCBDB* bdb, int thread_id, int key_index, int written, int base) {
  int query_index = (key_index % written) + base;
//int query_index = base; // always query the first thread-local record
  int ecode;
  char key[RECBUFSIZE];
  char* value;
  int keylen;
  int valuelen;
  keylen = sprintf(key, "%08d", query_index);
  memset(&key[keylen], 'a', RECBUFSIZE - keylen);
  key[RECBUFSIZE - 1] = '\0';

//  printf("%d: thread %d querying record with key: %d\n", key_index, thread_id, query_index);
//  printf("key (%zu bytes):\t%s\n", sizeof(key), key);
  pthread_mutex_lock(&lock);
  value = tcbdbget(bdb, key, RECBUFSIZE, &valuelen);
  if (!value) {
    pthread_mutex_unlock(&lock);
    int retry = 0;
    while (!value) {
      pthread_mutex_lock(&lock);
      /* Try the next query */
      query_index++;
      keylen = sprintf(key, "%08d", query_index);
      memset(&key[keylen], 'a', RECBUFSIZE - keylen);
      key[RECBUFSIZE - 1] = '\0';

      /* Issue query again */
      value = tcbdbget(bdb, key, RECBUFSIZE, &valuelen);
      ecode = tcbdbecode(bdb);
      fprintf(stderr, "Error %s.\n", tcbdberrmsg(ecode));
      fprintf(stderr, "%d: thread %d couldn't query record with key index: %d\n", key_index, thread_id, query_index);
      fprintf(stderr, "key: %s\n", key);
      pthread_mutex_unlock(&lock);
      traverse_btree(bdb, thread_id);
      retry++;
      if (retry > 10) {
        return false;
      }
    }
//  return false;
  } else {
//  printf("value (%d bytes):\t%s\n", valuelen, value);
    free(value);
    pthread_mutex_unlock(&lock);
  }

  return true;
}


/* thread read workload */
void* thread_read_worker(void* args) {
  struct thread_args* targs = (struct thread_args*)args;
  TCBDB* bdb = targs->bdb;
  int thread_id = targs->thread_id;
  int records;
  int base = thread_id * NRECORDS_PER_THREAD;
  int updatetotal = (NRECORDS_PER_THREAD * WRITE_PERCENT) / 100;
  int readtotal = NRECORDS_PER_THREAD - updatetotal;

  bool result;
  for (records = 0; records < readtotal; records++) {
    /* query record */
    result = query_record(bdb, thread_id, base + records, updatetotal, base);
    if (!result) {
      fprintf(stderr, "thread %d stop execution\n", thread_id);
      return NULL;
    }
  }
  printf("thread %d read %d records\n", thread_id, readtotal);
//if (thread_id == 0) {
//  traverse_btree(bdb, thread_id);
//}
  return NULL;
}
#endif

void update_record(TCBDB* bdb, int thread_id, int key_index) {
  int ecode;
  char key[RECBUFSIZE];
  char value[RECBUFSIZE];
  int keylen;
  bool result;

  /* generate a record */
  keylen = sprintf(key, "%08d", key_index);
  memset(&key[keylen], 'a', RECBUFSIZE - keylen);
  key[RECBUFSIZE - 1] = '\0';
  memset(value, 'b', RECBUFSIZE);
  value[RECBUFSIZE - 1] = '\0';

  /* insert a record */
  pthread_mutex_lock(&lock);
  printf("%d: thread %d updating key (%d bytes) / value (%d bytes):\n", key_index, thread_id, RECBUFSIZE, RECBUFSIZE);
  printf("key:\t%s\n", key);
  printf("value:\t%s\n", value); 
//result = tcbdbput(bdb, key, sizeof(key), value, sizeof(value));
//
//if (!result) {
//  ecode = tcbdbecode(bdb);
//  fprintf(stderr, "put error: %s\n", tcbdberrmsg(ecode));
//} else {
//  printf("%d: thread %d updated key (%d bytes) / value (%d bytes):\n", key_index, thread_id, RECBUFSIZE, RECBUFSIZE);
//  printf("key:\t%s\n", key);
//  printf("value:\t%s\n", value);
//}
  pthread_mutex_unlock(&lock);
}

/* thread write workload */
void* thread_update_worker(void* args) {
  struct thread_args* targs = (struct thread_args*)args;
  TCBDB* bdb = targs->bdb;
  int thread_id = targs->thread_id;
  int records;
  int base = thread_id * NRECORDS_PER_THREAD;
  int updatetotal = (NRECORDS_PER_THREAD * WRITE_PERCENT) / 100;

//printf("thread %d updating %d (out of %d) records, write percentage: %d%%\n", thread_id, updatetotal, NRECORDS_PER_THREAD, WRITE_PERCENT);
  for (records = 0; records < updatetotal; records++) {
    /* update a record */
    int key_index = rand() % (base + records);
//  int key_index = rand() % NRECORDS;
    update_record(bdb, thread_id, key_index);
  }
//printf("thread %d updated %d records, #records in bdb: %lu\n", thread_id, updatetotal, tcbdbrnum(bdb));
//if (thread_id == 0) {
//  traverse_btree(bdb, thread_id);
//}
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
//bdb = tcbdbnew();
  /* set mutual exclusion for threading*/
//if (!tcbdbsetmutex(bdb)) {
//  ecode = tcbdbecode(bdb);
//  fprintf(stderr, "setmutex error: %s\n", tcbdberrmsg(ecode));
//}

  /* open the database */
//if (!tcbdbopen(bdb, DATABASE, HDBOWRITER)) {
//  ecode = tcbdbecode(bdb);
//  fprintf(stderr, "open error: %s\n", tcbdberrmsg(ecode));
//  exit(-1);
//}

  /* create updater threads */
  
  printf("starts to write workload\n");
  pthread_t tid[NTHREADS];
  struct thread_args targs[NTHREADS];
  pthread_mutex_init(&lock, NULL);
  for (i = 0; i < NTHREADS; i++) {
    targs[i].thread_id = i;
    targs[i].bdb = bdb;
    pthread_create(&tid[i], NULL, thread_update_worker, &targs[i]);
  }
  for (i = 0; i < NTHREADS; i++) {
    pthread_join(tid[i], NULL);
  }
  
exit(0);

  /* create reader threads */
  /*
  printf("starts to read workload\n");
  for (i = 0; i < NTHREADS; i++) {
    targs[i].thread_id = i;
    targs[i].bdb = bdb;
    pthread_create(&tid[i], NULL, thread_read_worker, &targs[i]);
  }
  for (i = 0; i < NTHREADS; i++) {
    pthread_join(tid[i], NULL);
  }
*/

  /* traverse records */
  traverse_btree(bdb, 0);

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
