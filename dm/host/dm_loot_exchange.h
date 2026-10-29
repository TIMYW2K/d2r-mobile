#ifndef DM_LOOT_EXCHANGE_H
#define DM_LOOT_EXCHANGE_H
#include "dm_loot_capture.h"
#include <pthread.h>
#include <stdatomic.h>

// Experimental bridge only: it never hooks the game, hides native labels,
// schedules a display link, or draws. Allocate bridge and pair off-stack.
#define DM_LOOT_MAX_AGE_NS UINT64_C(150000000)
typedef struct {
    pthread_mutex_t lock;
    _Atomic uint64_t epoch;
    dm_loot_owned_snapshot previous, current;
} dm_loot_exchange;
typedef struct {
    uint64_t epoch;
    dm_loot_owned_snapshot previous, current;
} dm_loot_pair;

// Initialize once before starting threads; destroy after both have joined.
int dm_loot_exchange_init(dm_loot_exchange *x);
void dm_loot_exchange_destroy(dm_loot_exchange *x);
// Producer validates and atomically publishes a complete immutable copy.
int dm_loot_exchange_publish(dm_loot_exchange *x, const dm_loot_owned_snapshot *s);
// Scene/background/rotation owner calls invalidate before state is discarded.
void dm_loot_exchange_invalidate(dm_loot_exchange *x);
// Nonblocking UI handoff; caller owns pair storage. 0 means keep native draw.
int dm_loot_exchange_copy(dm_loot_exchange *x, dm_loot_pair *pair, uint64_t now_ns);
// Recheck after sampling and before any display/hit decision. This is a stale
// data guard, not a GPU presentation fence or a native-draw handshake.
int dm_loot_exchange_pair_valid(const dm_loot_exchange *x, const dm_loot_pair *pair);
// Produce a frame plan. The same returned rectangle must be used for painting
// and hit tests. Output style bytes are opaque and MUST NOT be dereferenced.
int dm_loot_pair_sample(const dm_loot_pair *pair, uint64_t now_ns,
                        dm_loot_owned_snapshot *out);
#endif
