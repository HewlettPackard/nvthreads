/* Copyright (c) 2007-2009, Stanford University
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*     * Neither the name of Stanford University nor the names of its 
*       contributors may be used to endorse or promote products derived from 
*       this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY STANFORD UNIVERSITY ``AS IS'' AND ANY
* EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL STANFORD UNIVERSITY BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/ 

#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <ctype.h>
#include <time.h>
#include <inttypes.h>

#include "stddefines.h"

#define SALT_SIZE 2
#define MAX_REC_LEN 1024
#define OFFSET 5

typedef struct {
  int keys_file_len;
  int encrypted_file_len;
  long bytes_comp;
  char * keys_file;
  char * encrypt_file;
  char* salt;
} str_data_t;

void compute_hashes(char* word, char* final_word);

/** getnextline()
 *  Function to get the next word
 */
int getnextline(char* output, int max_len, char* file)
{
	int i=0;
	while(i<max_len-1)
	{
		if( file[i] == '\0')
		{
			if(i==0)
				return -1;
			else
				return i;
		}
		if( file[i] == '\r')
			return (i+2);

		if( file[i] == '\n' )
			return (i+1);

		output[i] = file[i];
		i++;
	}
	file+=i;
	return i;
}

/** compute_hashes()
 *  Simple Cipher to generate a hash of the word 
 */
void compute_hashes(char* word, char* final_word)
{
	int i;

	for(i=0;i<strlen(word);i++) {
		final_word[i] = word[i]+OFFSET;
	}
	final_word[i] = '\0';
}

/** string_match()
 *  function that goes through file looking for matches to the given hashes 
 */
void string_match(str_data_t *data_in)
{
	assert(data_in);

	 char *key1 = "Helloworld";
	 char *key2 = "howareyou";
	 char *key3 = "ferrari";
	 char *key4 = "whotheman";

	 char *key1_final = malloc(strlen(key1) + 1);
	 char *key2_final = malloc(strlen(key2) + 1);
	 char *key3_final = malloc(strlen(key3) + 1);
	 char *key4_final = malloc(strlen(key4) + 1);

 //printf("%d %d\n", strlen(key1), strlen(key2));
	compute_hashes(key1, key1_final);
	compute_hashes(key2, key2_final);
	compute_hashes(key3, key3_final);
	compute_hashes(key4, key4_final);

	int key_len;
	char * key_file;
	key_file = data_in->keys_file;
	char * cur_word = malloc(MAX_REC_LEN);
	char * cur_word_final = malloc(MAX_REC_LEN);
	bzero(cur_word, MAX_REC_LEN);
	bzero(cur_word_final, MAX_REC_LEN);

    while( (key_len = getnextline(cur_word, MAX_REC_LEN, key_file))>=0)
    {
		compute_hashes(cur_word, cur_word_final);

	   if(!strcmp(key1_final, cur_word_final))
		   dprintf("FOUND: WORD IS %s\n", cur_word);

	   if(!strcmp(key2_final, cur_word_final))
		   dprintf("FOUND: WORD IS %s\n", cur_word);

	   if(!strcmp(key3_final, cur_word_final))
		   dprintf("FOUND: WORD IS %s\n", cur_word);

	   if(!strcmp(key4_final, cur_word_final))
		   dprintf("FOUND: WORD IS %s\n", cur_word);
		
	   key_file = key_file + key_len;
		bzero(cur_word, MAX_REC_LEN);
		bzero(cur_word_final, MAX_REC_LEN);
   }
   free(cur_word);
   free(cur_word_final);
   free(key1_final);
   free(key2_final);
   free(key3_final);
   free(key4_final);
}

int main(int argc, char *argv[]) 
{   
   int fd_keys;
   char *fdata_keys;
   struct stat finfo_keys;
   char *fname_keys;

	 /* Option to provide the encrypted words in a file as opposed to source code */
   //fname_encrypt = "encrypt.txt";
   
   if (argv[1] == NULL)
   {
      printf("USAGE: %s <keys filename>\n", argv[0]);
      exit(1);
   }
   fname_keys = argv[1];

   struct timeval starttime,endtime;
   srand( (unsigned)time( NULL ) );

   printf("String Match: Running...\n");

   /*// Read in the file
   CHECK_ERROR((fd_encrypt = open(fname_encrypt,O_RDONLY)) < 0);
   // Get the file info (for file length)
   CHECK_ERROR(fstat(fd_encrypt, &finfo_encrypt) < 0);
   // Memory map the file
   CHECK_ERROR((fdata_encrypt= mmap(0, finfo_encrypt.st_size + 1,
      PROT_READ | PROT_WRITE, MAP_PRIVATE, fd_encrypt, 0)) == NULL);*/

   // Read in the file
   CHECK_ERROR((fd_keys = open(fname_keys,O_RDONLY)) < 0);
   // Get the file info (for file length)
   CHECK_ERROR(fstat(fd_keys, &finfo_keys) < 0);
   // Memory map the file
   CHECK_ERROR((fdata_keys= mmap(0, finfo_keys.st_size + 1,
      PROT_READ | PROT_WRITE, MAP_PRIVATE, fd_keys, 0)) == NULL);

   // Setup splitter args

	//dprintf("Encrypted Size is %ld\n",finfo_encrypt.st_size);
	dprintf("Keys Size is %" PRId64 "\n",finfo_keys.st_size);

   str_data_t str_data;

   str_data.keys_file_len = finfo_keys.st_size;
   str_data.encrypted_file_len = 0;
   str_data.bytes_comp = 0;
   str_data.keys_file  = ((char *)fdata_keys);
   str_data.encrypt_file  = NULL;
   str_data.salt = malloc(SALT_SIZE);
   //str_data.encrypted_file_len = finfo_encrypt.st_size;
   //str_data.encrypt_file  = ((char *)fdata_encrypt);   

   //printf("String Match: Calling Serial String Match\n");
	 gettimeofday(&starttime,0);
   string_match(&str_data);
	 gettimeofday(&endtime,0);
   printf("String Match: Completed %ld\n",(endtime.tv_sec - starttime.tv_sec));

   free(str_data.salt);

   /*CHECK_ERROR(munmap(fdata_encrypt, finfo_encrypt.st_size + 1) < 0);
   CHECK_ERROR(close(fd_encrypt) < 0);*/

   CHECK_ERROR(munmap(fdata_keys, finfo_keys.st_size + 1) < 0);
   CHECK_ERROR(close(fd_keys) < 0);
   return 0;
}
