/*
  Copyright 2015-2016 Hewlett Packard Enterprise Development LP
  
  This program is free software; you can redistribute it and/or modify 
  it under the terms of the GNU General Public License, version 2 as 
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, 
  but WITHOUT ANY WARRANTY; without even the implied warranty of 
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
  See the GNU General Public License for more details.

  You should have received a copy of the GNU General Public License 
  along with this program; if not, write to the Free Software 
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

*/
#include "vector.h"
#include <mnemosyne.h>
#include <mtm.h>
#include <pmalloc.h>
//#ifndef PMALLOC
//#define pmalloc(sz) malloc(sz)
//#endif

extern void vector_f_init(vector_f *v, size_t size)
{
    vector_f_init_set(v, size, 0.0);
}

extern void vector_f_init_set(vector_f *v, size_t size, f_t init)
{
    assert(size > 0);
    size_t i;
    v->size = size;
    v->elements = malloc(sizeof(v->elements) * v->size);

	f_t sum = 0.0;

    for(i = 0; i < size; i++) {
        v->elements[i] = init;
	sum += init;
    }
}

extern void vector_i_init(vector_i *v, size_t size)
{
	vector_i_init_set(v, size, 0);
}

extern void vector_i_init_set(vector_i *v, size_t size, i_t init)
{
    assert(size > 0);
    assert(v != NULL);
    size_t i;
    v->size = size;
    v->elements = malloc(sizeof(v->elements) * size);

    for(i = 0; i < size; i++)
        v->elements[i] = init;
}

extern void vector_f_copy(vector_f *out, const vector_f *in)
{
    size_t i;
  	for (i = 0; i < out->size; i++)
		out->elements[i] = in->elements[i];
}

extern void vector_f_free(vector_f *v)
{
    assert(v != NULL);
    free(v->elements);
    v->elements = NULL;
    v->size = 0;
    v = NULL;
}

extern void vector_i_free(vector_i *v)
{
    assert(v != NULL);

    free(v->elements);
    v->elements = NULL;
    v->size = 0;
 
    v = NULL;
}

extern void vector_f_normalize(vector_f *v)
{
	assert(v != NULL);
    size_t i; 
	float sum = 0.0;
	for (i = 0; i < v->size; ++i)
	{
		sum += v->elements[i];
	}
	for (i = 0; i < v->size; ++i)
	{
		v->elements[i] /= sum;
	}
}

extern void vector_i_normalize(vector_i *v)
{
    assert(v != NULL);
    size_t i; 
    int sum = 0;

    for(i = 0; i < v->size; i++)
        sum += v->elements[i];

    if(sum != 0) {
        for(i = 0; i < v->size; i++)
            v->elements[i] /= sum;
    }
}

extern void vector_f_save(const char *path, const vector_f *v)
{
	assert (v != NULL);
    size_t i; 
    FILE *fp = NULL;
    fp = fopen(path, "wb");

	if(fp == NULL)
	{
		printf("IO error.\n");
		return;
	}
    for (i = 0; i < v->size; ++i)
    {
	fprintf(fp, "%.20f \n", v->elements[i]);
    }

    fclose(fp);

    printf("Successfully written '%s'\n", path);
}

