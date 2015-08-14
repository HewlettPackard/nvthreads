/*!
 * \file
 *
 * \author Haris Volos <hvolos@cs.wisc.edu>
 */
#include <stdio.h>
#include <mnemosyne.h>
#include <mtm.h>
#include <pmalloc.h>
#define size 4096 * 2
MNEMOSYNE_PERSISTENT int a[4096*2];
//MNEMOSYNE_PERSISTENT int flag = 0;

int main (int argc, char const *argv[])
{
    int i =0;
//  printf("flag: %d", flag);
    int *ptr = (int*)pmalloc(10000);
    ptr = prealloc(ptr, 20000);
    MNEMOSYNE_ATOMIC{
//  	if (flag == 0) {
//  		flag = 1;
//  	} else {
//  		flag = 0;
//  	}
        for (i = 0; i < size; i++) {
            fprintf(stderr, "writting to %d...", i);
            a[i] = 0;
            fprintf(stderr, "done\n");
        }
    }
	
//  printf(" --> %d\n", flag);
	return 0;
}
