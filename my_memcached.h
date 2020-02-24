#ifndef __MEM__
#define __MEM__
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <event.h>
#include <netdb.h>
#include <pthread.h>
#include <unistd.h>
#include <assert.h>
#include <grp.h>
#include <signal.h>



typedef unsigned int rel_time_t;

typedef signed char int8_t;
typedef unsigned char   uint8_t;
typedef short  int16_t;
typedef unsigned short  uint16_t;
typedef int  int32_t;
typedef unsigned   uint32_t;

#define HASHPOWER_DEFAULT 27
#define HASHPOWER_MAX 32

#define bool char
#define false 0
#define true 1

#define mutex_lock(x) pthread_mutex_lock(x)
#define mutex_unlock(x) pthread_mutex_unlock(x)

unsigned int hashpower;
unsigned int item_lock_hashpower;

static uint32_t item_lock_count;
static pthread_mutex_t *item_locks;


typedef struct _stritem {
    /* Protected by LRU locks */
    struct _stritem *next;
    struct _stritem *prev;
    /* Rest are protected by an item lock */
    struct _stritem *h_next;    /* hash chain next */
    rel_time_t      time;       /* least recent access */
    rel_time_t      exptime;    /* expire time */
    int             nbytes;     /* size of data */
    unsigned short  refcount;
    uint16_t        it_flags;   /* ITEM_* above */
    uint8_t         slabs_clsid;/* which slab class we're in */
    uint8_t         nkey;       /* key length, w/terminating null and padding */
    /* this odd type prevents type-punning issues when we do
     * the little shuffle to save space when not using CAS. */
    union {
        uint64_t cas;
        char end;
    } data[];
    /* if it_flags & ITEM_CAS we have 8 bytes CAS */
    /* then null-terminated key */
    /* then " flags length\r\n" (no terminating null) */
    /* then data with terminating \r\n (no terminating null; it's binary!) */
} item;

#define NULL ((void *)0)


struct stats_state {
    uint64_t      curr_items;
    uint64_t      curr_bytes;
    uint64_t      curr_conns;
    uint64_t      hash_bytes;       /* size used for hash tables */
    unsigned int  conn_structs;
    unsigned int  reserved_fds;
    unsigned int  hash_power_level; /* Better hope it's not over 9000 */
    bool          hash_is_expanding; /* If the hash table is being expanded */
    bool          accepting_conns;  /* whether we are currently accepting */
    bool          slab_reassign_running; /* slab reassign in progress */
    bool          lru_crawler_running; /* crawl in progress */
};

struct stats_state stats_state;

#ifndef _SIG_ATOMIC_T_DEFINED
#define _SIG_ATOMIC_T_DEFINED
  typedef int sig_atomic_t;
#endif

enum protocol {
    ascii_prot = 3, /* arbitrary value. */
    binary_prot,
    negotiating_prot /* Discovering the protocol */
};
struct settings {
    size_t maxbytes;
    int maxconns;
    int port;
    int udpport;
    char *inter;
    int verbose;
    rel_time_t oldest_live; /* ignore existing items older than this */
    uint64_t oldest_cas; /* ignore existing items with CAS values lower than this */
    int evict_to_free;
    char *socketpath;   /* path to unix socket if using local socket */
    char *auth_file;    /* path to user authentication file */
    int access;  /* access mask (a la chmod) for unix domain socket */
    double factor;          /* chunk size growth factor */
    int chunk_size;
    int num_threads;        /* number of worker (without dispatcher) libevent threads to run */
    int num_threads_per_udp; /* number of worker threads serving each udp socket */
    char prefix_delimiter;  /* character that marks a key prefix (for stats) */
    int detail_enabled;     /* nonzero if we're collecting detailed stats */
    int reqs_per_event;     /* Maximum number of io to process on each
                               io-event. */
    bool use_cas;
    enum protocol binding_protocol;
    int backlog;
    int item_size_max;        /* Maximum item size */
    int slab_chunk_size_max;  /* Upper end for chunks within slab pages. */
    int slab_page_size;     /* Slab's page units. */
    volatile sig_atomic_t sig_hup;  /* a HUP signal was received but not yet handled */
    bool sasl;              /* SASL on/off */
    bool maxconns_fast;     /* Whether or not to early close connections */
    bool lru_crawler;        /* Whether or not to enable the autocrawler thread */
    bool lru_maintainer_thread; /* LRU maintainer background thread */
    bool lru_segmented;     /* Use split or flat LRU's */
    bool slab_reassign;     /* Whether or not slab reassignment is allowed */
    int slab_automove;     /* Whether or not to automatically move slabs */
    double slab_automove_ratio; /* youngest must be within pct of oldest */
    unsigned int slab_automove_window; /* window mover for algorithm */
    int hashpower_init;     /* Starting hash power level */
    bool shutdown_command; /* allow shutdown command */
    int tail_repair_time;   /* LRU tail refcount leak repair time */
    bool flush_enabled;     /* flush_all enabled */
    bool dump_enabled;      /* whether cachedump/metadump commands work */
    char *hash_algorithm;     /* Hash algorithm in use */
    int lru_crawler_sleep;  /* Microsecond sleep between items */
    uint32_t lru_crawler_tocrawl; /* Number of items to crawl per run */
    int hot_lru_pct; /* percentage of slab space for HOT_LRU */
    int warm_lru_pct; /* percentage of slab space for WARM_LRU */
    double hot_max_factor; /* HOT tail age relative to COLD tail */
    double warm_max_factor; /* WARM tail age relative to COLD tail */
    int crawls_persleep; /* Number of LRU crawls to run before sleeping */
    bool temp_lru; /* TTL < temporary_ttl uses TEMP_LRU */
    uint32_t temporary_ttl; /* temporary LRU threshold */
    int idle_timeout;       /* Number of seconds to let connections idle */
    unsigned int logger_watcher_buf_size; /* size of logger's per-watcher buffer */
    unsigned int logger_buf_size; /* size of per-thread logger buffer */
    bool drop_privileges;   /* Whether or not to drop unnecessary process privileges */
    bool relaxed_privileges;   /* Relax process restrictions when running testapp */
#ifdef EXTSTORE
    unsigned int ext_item_size; /* minimum size of items to store externally */
    unsigned int ext_item_age; /* max age of tail item before storing ext. */
    unsigned int ext_low_ttl; /* remaining TTL below this uses own pages */
    unsigned int ext_recache_rate; /* counter++ % recache_rate == 0 > recache */
    unsigned int ext_wbuf_size; /* read only note for the engine */
    unsigned int ext_compact_under; /* when fewer than this many pages, compact */
    unsigned int ext_drop_under; /* when fewer than this many pages, drop COLD items */
    double ext_max_frag; /* ideal maximum page fragmentation */
    double slab_automove_freeratio; /* % of memory to hold free as buffer */
    bool ext_drop_unread; /* skip unread items during compaction */
    /* per-slab-class free chunk limit */
    unsigned int ext_free_memchunks[MAX_NUMBER_OF_SLAB_CLASSES];
#endif
#ifdef TLS
    bool ssl_enabled; /* indicates whether SSL is enabled */
    SSL_CTX *ssl_ctx; /* holds the SSL server context which has the server certificate */
    char *ssl_chain_cert; /* path to the server SSL chain certificate */
    char *ssl_key; /* path to the server key */
    int ssl_verify_mode; /* client certificate verify mode */
    int ssl_keyformat; /* key format , defult is PEM */
    char *ssl_ciphers; /* list of SSL ciphers */
    char *ssl_ca_cert; /* certificate with CAs. */
    rel_time_t ssl_last_cert_refresh_time; /* time of the last server certificate refresh */
    unsigned int ssl_wbuf_size; /* size of the write buffer used by ssl_sendmsg method */
#endif
};
extern struct settings settings;



enum pause_thread_types {
    PAUSE_WORKER_THREADS = 0,
    PAUSE_ALL_THREADS,
    RESUME_ALL_THREADS,
    RESUME_WORKER_THREADS
};


void item_lock(uint32_t hv);
void *item_trylock(uint32_t hv);
void item_trylock_unlock(void *lock);
void item_unlock(uint32_t hv);

void item_lock_init();

void STATS_LOCK();
void STATS_UNLOCK();

#define ITEM_CAS 2
#define ITEM_CFLAGS 256

#define ITEM_key(item) (((char*)&((item)->data)) \
         + (((item)->it_flags & ITEM_CAS) ? sizeof(uint64_t) : 0))
#define ITEM_key(item) (((char*)&((item)->data)) \
         + (((item)->it_flags & ITEM_CAS) ? sizeof(uint64_t) : 0))

#define ITEM_suffix(item) ((char*) &((item)->data) + (item)->nkey + 1 \
         + (((item)->it_flags & ITEM_CAS) ? sizeof(uint64_t) : 0))

#define ITEM_data(item) ((char*) &((item)->data) + (item)->nkey + 1 \
         + (((item)->it_flags & ITEM_CFLAGS) ? sizeof(uint32_t) : 0) \
         + (((item)->it_flags & ITEM_CAS) ? sizeof(uint64_t) : 0))

#define MEMCACHED_ASSOC_FIND(arg0, arg1, arg2)
#define MEMCACHED_ASSOC_INSERT(arg0, arg1)
#define MEMCACHED_ASSOC_DELETE(arg0, arg1)
#endif
