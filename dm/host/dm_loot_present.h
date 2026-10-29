#ifndef DM_LOOT_PRESENT_H
#define DM_LOOT_PRESENT_H
#include "dm_loot_capture.h"
#include <pthread.h>

// Device-side handshake prototype. A successful commit means the caller has
// actually rendered the exact eligible labels; a mere scheduled draw is not
// enough. A matching native draw can then be suppressed for a short time.
// Synchronizing a CPU callback with actual GPU presentation remains the iOS
// renderer's responsibility; this module does not itself draw any pixels.
#define DM_LOOT_PRESENT_MAX_AGE_NS UINT64_C(100000000)
typedef struct {
    pthread_mutex_t lock;
    uint64_t epoch, committed_ns;
    dm_loot_owned_snapshot visible;
} dm_loot_present;
int dm_loot_present_init(dm_loot_present *p);
void dm_loot_present_destroy(dm_loot_present *p);
void dm_loot_present_invalidate(dm_loot_present *p);
int dm_loot_present_commit(dm_loot_present *p,const dm_loot_owned_snapshot *frame,
                           const unsigned char *eligible,uint64_t epoch,uint64_t ns);
int dm_loot_present_can_hide(dm_loot_present *p,const void *record,size_t bytes,
                             uint64_t epoch,uint64_t now,float ui_dy);
int dm_loot_present_hit(dm_loot_present *p,float x,float y,float min_size,
                        uint64_t epoch,uint64_t now,uint32_t *id);
#endif
