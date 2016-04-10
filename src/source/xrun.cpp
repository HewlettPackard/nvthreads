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


#include "xrun.h"

size_t xrun::_master_thread_id;
size_t xrun::_thread_index;
bool xrun::_fence_enabled;

char *xrun::logPath;
volatile bool xrun::_initialized = false;
volatile bool xrun::_protection_enabled = false;
volatile bool xrun::_protection_again = false;
size_t xrun::_children_threads_count = 0;
size_t xrun::_lock_count = 0;
bool xrun::_token_holding = false;
static MemoryLog _localMemoryLog; 
static nvrecovery _localNvRecovery;
