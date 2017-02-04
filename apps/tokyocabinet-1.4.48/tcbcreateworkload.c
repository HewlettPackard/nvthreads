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

void insert_record(TCBDB* bdb, int key_index, int keysize) {
  int ecode;
  char *key = (char*)malloc(keysize);
  char *value = (char*)malloc(keysize);
  int keylen;
  bool result;

  /* generate a record */
  keylen = sprintf(key, "%08d", key_index);
  memset(&key[keylen], 'a', keysize - keylen);
  key[keysize - 1] = '\0';
  memset(value, 'b', keysize);
  value[keysize - 1] = '\0';

  /* insert a record */
//result = tcbdbput(bdb, key, keysize, value, keysize);
  result = tcbdbput2(bdb, key, value);
  if (!result) {
    ecode = tcbdbecode(bdb);
    fprintf(stderr, "put error: %s\n", tcbdberrmsg(ecode));
  } else {
//  printf("%d: inserted key (%d bytes) / value (%d bytes):\n", key_index, keysize, keysize);
//  printf("key:\t%s\n", key);
//  printf("value:\t%s\n", value);
  }
}

bool create_database(int backend, int nrecords, int keysize) {
  TCBDB* bdb;
  int ecode;
  char filename[128];
  int i;
  int ret;

  /* Decide filename and localtion for the database */
  if (backend == HD) {
    sprintf(filename, "./workload-%drecords-%dbytes.bdb", nrecords, keysize);
  } else if (backend == SSD) {
    sprintf(filename, "/mnt/ssd/workload-%drecords-%dbytes.bdb", nrecords, keysize);
  } else if (backend == TMPFS) {
    sprintf(filename, "/tmp/ramdisk/workload-%drecords-%dbytes.bdb", nrecords, keysize);
  } else if (backend == NVMFS) {
    sprintf(filename, "/mnt/ramdisk/workload-%drecords-%dbytes.bdb", nrecords, keysize);
  }
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
  if (!tcbdbopen(bdb, filename, BDBOWRITER | BDBOCREAT)) {
    ecode = tcbdbecode(bdb);
    fprintf(stderr, "open error: %s\n", tcbdberrmsg(ecode));
    return false;
  }

  /* generate 100,000 records */
  for (i = 0; i < nrecords; i++) {
    insert_record(bdb, i, keysize);
  }

  /* traverse records */
//traverse_btree(bdb, 0);

  /* close the database */
  printf("closing databse\n");
  if (!tcbdbclose(bdb)) {
    ecode = tcbdbecode(bdb);
    fprintf(stderr, "close error: %s\n", tcbdberrmsg(ecode));
  }
  /* delete the object */
  printf("deleting databse\n");
  tcbdbdel(bdb);

  printf("database created at: %s\n", filename);
  return true;
}

int main(int argc, char** argv) {

  int backend;
  int nrecords;
  int keysize;
  bool result;

  /* check input */
  if (argc < 4) {
    fprintf(stderr, "Usage: %s [HD/SSD/TMPFS/NVMFS] [nrecords] [keysize]\n", argv[0]);
    exit(-1);
  }

  if (strcmp("HD", argv[1]) == 0) {
    backend = HD;
  } else if (strcmp("SSD", argv[1]) == 0) {
    backend = SSD;
  } else if (strcmp("TMPFS", argv[1]) == 0) {
    backend = TMPFS;
  } else if (strcmp("NVMFS", argv[1]) == 0) {
    backend = NVMFS;
  } else {
    fprintf(stderr, "Usage: %s [HD/SSD/TMPFS/NVMFS] [nrecords] [keysize]\n", argv[0]);
    exit(-1);
  }
  nrecords = atoi(argv[2]);
  keysize = atoi(argv[3]);

  result = create_database(backend, nrecords, keysize);
  if (!result) {
    fprintf(stderr, "error: failed to create database\n");
    exit(-1);
  }

  return 0;
}
