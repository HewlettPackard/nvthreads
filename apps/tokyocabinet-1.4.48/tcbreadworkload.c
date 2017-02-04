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
enum {
  HD,
  SSD,
  TMPFS,
  NVMFS
};

/* list out all records in key order */
void traverse_btree(TCBDB* bdb, int thread_id) {
  BDBCUR* cur;
  char* key, *value;
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
}

int main(int argc, char** argv) {

  /* check input */
  if (argc < 2) {
    fprintf(stderr, "Usage: %s [database]\n", argv[0]);
    exit(-1);
  }

  TCBDB* bdb;
  int ecode;

  /* create the object */
  bdb = tcbdbnew();

  /* open the database */
  if (!tcbdbopen(bdb, argv[1], BDBOREADER)) {
    ecode = tcbdbecode(bdb);
    fprintf(stderr, "open error: %s\n", tcbdberrmsg(ecode));
    return false;
  }

  /* traverse records */
  traverse_btree(bdb, 0);

  /* close the database */
  printf("closing databse\n");
  if (!tcbdbclose(bdb)) {
    ecode = tcbdbecode(bdb);
    fprintf(stderr, "close error: %s\n", tcbdberrmsg(ecode));
  }
  /* delete the object */
  printf("deleting databse\n");
  tcbdbdel(bdb); 

  return 0;
}
