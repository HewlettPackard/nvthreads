/* Copyright (C) 2004 Christopher Clark <firstname.lastname@cl.cam.ac.uk> */

#include "hashtable.h"
#include "hashtable_private.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdint.h>

#include "atlas_api.h"
#include "atlas_alloc.h"

/*
Credit for primes table: Aaron Krowne
 http://br.endernet.org/~akrowne/
 http://planetmath.org/encyclopedia/GoodHashTablePrimes.html
*/
static const unsigned int primes[] = {
53, 97, 193, 389,
769, 1543, 3079, 6151,
12289, 24593, 49157, 98317,
196613, 393241, 786433, 1572869,
3145739, 6291469, 12582917, 25165843,
50331653, 100663319, 201326611, 402653189,
805306457, 1610612741
};
const unsigned int prime_table_length = sizeof(primes)/sizeof(primes[0]);
const float max_load_factor = 0.65;

extern uint32_t hash_rgn_id;

/*****************************************************************************/
struct hashtable *
create_hashtable(unsigned int minsize,
                 unsigned int (*hashf) (void*),
                 int (*eqf) (void*,void*))
{
    struct hashtable *h;
    unsigned int pindex, size = primes[0];
    /* Check requested hashtable isn't too large */
    if (minsize > (1u << 30)) return NULL;
    /* Enforce size as prime */
    for (pindex=0; pindex < prime_table_length; pindex++) {
        if (primes[pindex] > minsize) { size = primes[pindex]; break; }
    }

    // not required, since the hashtable is not yet exposed
//    NVM_BEGIN_DURABLE();
    
    h = (struct hashtable *)nvm_alloc(sizeof(struct hashtable), hash_rgn_id);
    if (NULL == h) return NULL; /*oom*/
    h->table = (struct entry **)nvm_alloc(sizeof(struct entry*) * size,
                                          hash_rgn_id);
    if (NULL == h->table) { nvm_free(h); return NULL; } /*oom*/

//    memset(h->table, 0, size * sizeof(struct entry *)); // TODO
    NVM_MEMSET(h->table, 0, size * sizeof(struct entry *));

    NVM_STR2(h->tablelength, size, sizeof(h->tablelength)*8);

    NVM_STR2(h->primeindex, pindex, sizeof(h->primeindex)*8);

    NVM_STR2(h->entrycount, 0, sizeof(h->entrycount)*8);

    h->hashfn = hashf;
    h->eqfn = eqf;
    
    NVM_STR2(h->loadlimit, (unsigned int) ceil(size * max_load_factor),
             sizeof(h->loadlimit)*8);

//    NVM_END_DURABLE();
    
    return h;
}

/*****************************************************************************/
unsigned int
hash(struct hashtable *h, void *k)
{
    /* Aim to protect against poor hash functions by adding logic here
     * - logic taken from java 1.4 hashtable source */
    unsigned int i = h->hashfn(k);
    i += ~(i << 9);
    i ^=  ((i >> 14) | (i << 18)); /* >>> */
    i +=  (i << 4);
    i ^=  ((i >> 10) | (i << 22)); /* >>> */
    return i;
}

/*****************************************************************************/
static int
hashtable_expand(struct hashtable *h)
{
    /* Double the size of the table to accomodate more entries */
    struct entry **newtable;
    struct entry *e;
    struct entry **pE;
    unsigned int newsize, i, index;
    /* Check we're not hitting max capacity */
    if (h->primeindex == (prime_table_length - 1)) return 0;
    newsize = primes[++(h->primeindex)];

    newtable = (struct entry **)nvm_alloc(sizeof(struct entry*) * newsize,
                                          hash_rgn_id);
    if (NULL != newtable)
    {
//        memset(newtable, 0, newsize * sizeof(struct entry *)); // TODO
        NVM_MEMSET(newtable, 0, newsize * sizeof(struct entry *));
        
        NVM_BEGIN_DURABLE();
    
        /* This algorithm is not 'stable'. ie. it reverses the list
         * when it transfers entries between the tables */
        for (i = 0; i < h->tablelength; i++) {
            while (NULL != (e = h->table[i])) {
                NVM_STR2(h->table[i], e->next, sizeof(h->table[i])*8);
                
                index = indexFor(newsize,e->h);

                NVM_STR2(e->next, newtable[index], sizeof(e->next)*8);
                
                NVM_STR2(newtable[index], e, sizeof(newtable[index])*8);
            }
        }
        nvm_free(h->table);

        NVM_STR2(h->table, newtable, sizeof(h->table)*8);
    }
    /* Plan B: realloc instead */
    else // TODO not handled in the NVM version
    {
        newtable = (struct entry **)
                   realloc(h->table, newsize * sizeof(struct entry *));
        if (NULL == newtable) { (h->primeindex)--; return 0; }
        h->table = newtable;
        memset(newtable[h->tablelength], 0, newsize - h->tablelength);
        for (i = 0; i < h->tablelength; i++) {
            for (pE = &(newtable[i]), e = *pE; e != NULL; e = *pE) {
                index = indexFor(newsize,e->h);
                if (index == i)
                {
                    pE = &(e->next);
                }
                else
                {
                    *pE = e->next;
                    e->next = newtable[index];
                    newtable[index] = e;
                }
            }
        }
    }
    NVM_STR2(h->tablelength, newsize, sizeof(h->tablelength)*8);
    NVM_STR2(h->loadlimit, (unsigned int) ceil(newsize * max_load_factor),
             sizeof(h->loadlimit)*8);
    NVM_END_DURABLE();
    
    return -1;
}

/*****************************************************************************/
unsigned int
hashtable_count(struct hashtable *h)
{
    return h->entrycount;
}

/*****************************************************************************/
int
hashtable_insert(struct hashtable *h, void *k, void *v)
{
    /* This method allows duplicate keys - but they shouldn't be used */
    unsigned int index;
    struct entry *e;

    e = (struct entry *)nvm_alloc(sizeof(struct entry), hash_rgn_id);
    if (NULL == e) {
        NVM_STR2(h->entrycount, h->entrycount - 1, sizeof(h->entrycount)*8);
        return 0;
    } /*oom*/
    e->h = hash(h,k);
    NVM_STR2(e->h, hash(h,k), sizeof(e->h)*8);
    NVM_STR2(e->k, k, sizeof(e->k)*8);

    NVM_STR2(e->v, v, sizeof(e->v)*8);

    if (h->entrycount+1 > h->loadlimit)
    {
        /* Ignore the return value. If expand fails, we should
         * still try cramming just this value into the existing table
         * -- we may not have memory for a larger table, but one more
         * element may be ok. Next time we insert, we'll try expanding again.*/
        hashtable_expand(h);
    }
    index = indexFor(h->tablelength,e->h);

    NVM_BEGIN_DURABLE();
    
    NVM_STR2(h->entrycount, h->entrycount + 1, sizeof(h->entrycount)*8);
    
    NVM_STR2(e->next, h->table[index], sizeof(e->next)*8);

    NVM_STR2(h->table[index], e, sizeof(h->table[index])*8);

    NVM_END_DURABLE();
    
    return -1;
}

/*****************************************************************************/
void * /* returns value associated with key */
hashtable_search(struct hashtable *h, void *k)
{
    struct entry *e;
    unsigned int hashvalue, index;
    hashvalue = hash(h,k);
    index = indexFor(h->tablelength,hashvalue);
    e = h->table[index];
    while (NULL != e)
    {
        /* Check hash value to short circuit heavier comparison */
        if ((hashvalue == e->h) && (h->eqfn(k, e->k))) return e->v;
        e = e->next;
    }
    return NULL;
}

/*****************************************************************************/
void * /* returns value associated with key */
hashtable_remove(struct hashtable *h, void *k)
{
    /* TODO: consider compacting the table when the load factor drops enough,
     *       or provide a 'compact' method. */

    struct entry *e;
    struct entry **pE;
    void *v;
    unsigned int hashvalue, index;

    hashvalue = hash(h,k);
    index = indexFor(h->tablelength,hash(h,k));
    pE = &(h->table[index]);
    e = *pE;

    NVM_BEGIN_DURABLE();
    
    while (NULL != e)
    {
        /* Check hash value to short circuit heavier comparison */
        if ((hashvalue == e->h) && (h->eqfn(k, e->k)))
        {
            NVM_STR2(*pE, e->next, sizeof(*pE)*8);
            
            NVM_STR2(h->entrycount, h->entrycount - 1,
                     sizeof(h->entrycount)*8);

            v = e->v;
            
            freekey(e->k);
            nvm_free(e);

            NVM_END_DURABLE();

            return v;
        }
        pE = &(e->next);
        e = e->next;
    }

    NVM_END_DURABLE();
    return NULL;
}

/*****************************************************************************/
/* destroy */
void
hashtable_destroy(struct hashtable *h, int free_values)
{
    unsigned int i;
    struct entry *e, *f;
    struct entry **table = h->table;
    if (free_values)
    {
        for (i = 0; i < h->tablelength; i++)
        {
            e = table[i];
            while (NULL != e)
            {
                f = e;
                e = e->next;
                freekey(f->k);
                nvm_free(f->v);
                nvm_free(f);
            }
        }
    }
    else
    {
        for (i = 0; i < h->tablelength; i++)
        {
            e = table[i];
            while (NULL != e)
            {
                f = e;
                e = e->next;
                freekey(f->k);
                nvm_free(f);
            }
        }
    }
    nvm_free(h->table);
    nvm_free(h);
}

/*
 * Copyright (c) 2002, Christopher Clark
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * 
 * * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * 
 * * Neither the name of the original author; nor the names of any contributors
 * may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 * 
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
