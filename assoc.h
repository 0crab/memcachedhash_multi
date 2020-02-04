/* associative array */
#ifndef __ASSOC__
#define __ASSOC__
#include "my_memcached.h"
void assoc_init(const int hashpower_init);
item *assoc_find(const char *key, const size_t nkey, const uint32_t hv);
int assoc_insert(item *item, const uint32_t hv);
void assoc_delete(const char *key, const size_t nkey, const uint32_t hv);
void do_assoc_move_next_bucket(void);
int start_assoc_maintenance_thread(void);
void stop_assoc_maintenance_thread(void);
void assoc_start_expand(uint64_t curr_items);
bool my_memcached_set(uint32_t hv,item * new_it);

void pause_threads(enum pause_thread_types type);

#endif