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
