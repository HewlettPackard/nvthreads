/*
    Copyright (C) 2011 Computer Sciences Department, 
    University of Wisconsin -- Madison

    ----------------------------------------------------------------------

    This file is part of Mnemosyne: Lightweight Persistent Memory, 
    originally developed at the University of Wisconsin -- Madison.

    Mnemosyne was originally developed primarily by Haris Volos
    with contributions from Andres Jaan Tack.

    ----------------------------------------------------------------------

    Mnemosyne is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation, version 2
    of the License.
 
    Mnemosyne is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, 
    Boston, MA  02110-1301, USA.

### END HEADER ###
*/

#ifndef _MNEMOSYNE_PMALLOC_H
#define _MNEMOSYNE_PMALLOC_H

#include <stdlib.h>

#if __cplusplus
extern "C" {
#endif

void *pmalloc(size_t sz);
void pfree(void *);
void *prealloc(void *, size_t);

//__attribute__((tm_callable)) void * pmalloc (size_t sz);
__attribute__((tm_wrapping(pmalloc))) void *pmallocTxn(size_t);
__attribute__((tm_wrapping(pfree))) void pfreeTxn(void *);
__attribute__((tm_wrapping(prealloc))) void *preallocTxn(void *, size_t);

#if __cplusplus
}
#endif

#endif
