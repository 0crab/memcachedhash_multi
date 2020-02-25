#include "my_memcached.h"
#include <pthread.h>
#include <stdlib.h>




typedef  uint32_t  ub4;  

#define hashsize(n) ((ub4)1<<(n))
#define hashmask(n) (hashsize(n)-1)



void item_lock_init(int nthreads){
    item_locks = calloc(item_lock_count, sizeof(pthread_mutex_t));
    if (! item_locks) {
        perror("Can't allocate item locks");
        exit(1);
    }
    for (int i = 0; i < item_lock_count; i++) {
        pthread_mutex_init(&item_locks[i], NULL);
    }
    int power;
    if (nthreads < 3) {
        power = 10;
    } else if (nthreads < 4) {
        power = 11;
    } else if (nthreads < 5) {
        power = 12;
    } else if (nthreads <= 10) {
        power = 13;
    } else if (nthreads <= 20) {
        power = 14;
    } else {
        /* 32k buckets. just under the hashpower default. */
        power = 15;
    }

    item_lock_count = hashsize(power);
    item_lock_hashpower = power;
    item_locks = calloc(item_lock_count, sizeof(pthread_mutex_t));

    for (int i = 0; i < item_lock_count; i++) {
        pthread_mutex_init(&item_locks[i], NULL);
    }

}

void item_lock(uint32_t hv) {
    mutex_lock(&item_locks[hv & hashmask(item_lock_hashpower)]);
}

void *item_trylock(uint32_t hv) {
    pthread_mutex_t *lock = &item_locks[hv & hashmask(item_lock_hashpower)];
    if (pthread_mutex_trylock(lock) == 0) {
        return lock;
    }
    return NULL;
}

void item_trylock_unlock(void *lock) {
    mutex_unlock((pthread_mutex_t *) lock);
}

void item_unlock(uint32_t hv) {
    mutex_unlock(&item_locks[hv & hashmask(item_lock_hashpower)]);
}

/* Lock for global stats */
static pthread_mutex_t stats_lock = PTHREAD_MUTEX_INITIALIZER;

void STATS_LOCK() {
    pthread_mutex_lock(&stats_lock);
}

void STATS_UNLOCK() {
    pthread_mutex_unlock(&stats_lock);
}
struct settings settings;