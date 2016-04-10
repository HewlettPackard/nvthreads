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

#ifndef VECTOR_H
#define VECTOR_H

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include <types.h>

typedef struct vector_f {
    f_t* elements;
    size_t size;
} vector_f;

typedef struct vector_i {
    i_t* elements;
    size_t size;
} vector_i;

extern void vector_f_init(vector_f *v, size_t size);

extern void vector_f_init_set(vector_f *v, size_t size, f_t init);

extern void vector_i_init(vector_i *v, size_t size);

extern void vector_i_init_set(vector_i *v, size_t size, i_t init);

extern void vector_f_copy(vector_f *out, const vector_f *in);

extern void vector_f_free(vector_f *v);

extern void vector_i_free(vector_i *v);

extern void vector_f_normalize(vector_f *v);

extern void vector_i_normalize(vector_i *v);

extern void vector_f_save(const char *path, const vector_f *v);

#endif // VECTOR_H

