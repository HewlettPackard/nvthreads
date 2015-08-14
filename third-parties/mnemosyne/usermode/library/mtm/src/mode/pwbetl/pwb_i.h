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

/**
 * \file pwb_i.h
 *
 * \brief Private header file for write-back with encounter-time locking.
 *
 */

#ifndef _PWBNL_INTERNAL_HJA891_H
#define _PWBNL_INTERNAL_HJA891_H

#undef  DESIGN
#define DESIGN WRITE_BACK_ETL


#include "mode/pwb-common/pwb_i.h"

#include "mode/common/mask.h"
#include "mode/common/barrier.h"
#include "mode/pwbetl/barrier.h"
#include "mode/pwbetl/beginend.h"
#include "mode/pwbetl/memcpy.h"
#include "mode/pwbetl/memset.h"

#include "mode/common/memcpy.h"
#include "mode/common/memset.h"
#include "mode/common/rwset.h"

#include "cm.h"


#endif /* _PWBNL_INTERNAL_HJA891_H */
