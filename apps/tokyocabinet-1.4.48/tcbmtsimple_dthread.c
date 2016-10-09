#include <tcutil.h>
#include <tcbdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>
#include <sys/time.h>
#define NTHREADS 8
#define NRECORDS (NTHREADS * 10)
#define NRECORDS_PER_THREAD (NRECORDS / NTHREADS)
#define RECBUFSIZ 64

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

void* thread_read(void* args) {
  return NULL;
}

/* write workload */
void* thread_write(void* args) {
  struct thread_args* targs = (struct thread_args*)args;
  TCBDB* bdb = targs->bdb;
  int thread_id = targs->thread_id;
  int ecode;
  int records;
  int base = thread_id * NRECORDS;
  char buf[RECBUFSIZ];

  for (records = 1; records <= NRECORDS_PER_THREAD; records++) {
    /* generate a record */
    int len = sprintf(buf, "%08d", base + rand() % (NRECORDS * 10));

    /* store a record */
    pthread_mutex_lock(&lock);
    printf("thread %d putting %d-th (key/value: %s)\n", thread_id, records, buf);
    if (!tcbdbput(bdb, buf, len, buf, len)) {
      ecode = tcbdbecode(bdb);
      fprintf(stderr, "put error: %s\n", tcbdberrmsg(ecode));
    } 
    pthread_mutex_unlock(&lock);
  }
  return NULL;
}

int main(int argc, char** argv) {
  TCBDB* bdb;
  int ecode;
  int i;
  pthread_t tid[NTHREADS];
  struct thread_args targs[NTHREADS];
  struct timeval start, end;
  unsigned long long elapsed;

  srand(time(NULL));

  gettimeofday(&start, NULL);

  /* create the object */
  bdb = tcbdbnew();
  /* set mutual exclusion for threading*/
  if (!tcbdbsetmutex(bdb)) {
    ecode = tcbdbecode(bdb);
    fprintf(stderr, "setmutex error: %s\n", tcbdberrmsg(ecode));
  }
  /* open the database */
  if (!tcbdbopen(bdb, "casket.bdb", HDBOWRITER | HDBOCREAT | BDBOTSYNC)) {
    ecode = tcbdbecode(bdb);
    fprintf(stderr, "open error: %s\n", tcbdberrmsg(ecode));
  }
  /* create threads */
  pthread_mutex_init(&lock, NULL);
  for (i = 0; i < NTHREADS; i++) {
    targs[i].thread_id = i;
    targs[i].bdb = bdb;
    pthread_create(&tid[i], NULL, thread_write, &targs[i]);
  }
  for (i = 0; i < NTHREADS; i++) {
    pthread_join(tid[i], NULL);
  }

  /* traverse records */
  traverse_btree(&targs[0]);

  /* close the database */
  if (!tcbdbclose(bdb)) {
    ecode = tcbdbecode(bdb);
    fprintf(stderr, "close error: %s\n", tcbdberrmsg(ecode));
  }
  /* delete the object */
  tcbdbdel(bdb);

  gettimeofday(&end, NULL);
  elapsed = 1000 * (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000;

  printf("%llu ms\n", elapsed);
  return 0;
}
