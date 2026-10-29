#ifndef DM_LOOT_LIVE_H
#define DM_LOOT_LIVE_H
#include "dm_loot_capture.h"
// UI commits only labels actually drawn, then the game may hide matching native
// calls. All failures restore the original drawing and original hit testing.
void dm_loot_live_invalidate(void);
int dm_loot_live_commit(const dm_loot_owned_snapshot *,const unsigned char *,
                        uint64_t epoch,uint64_t ns);
int dm_loot_live_hide(const void *record,uint64_t ns);
int dm_loot_live_hit(float x,float y,float min_size,uint64_t ns,uint32_t *id);
void dm_loot_live_stats(uint64_t *commits,uint64_t *eligible,uint64_t *seen,uint64_t *hidden);
#endif
