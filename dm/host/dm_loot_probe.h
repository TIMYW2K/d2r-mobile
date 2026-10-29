#ifndef DM_LOOT_PROBE_H
#define DM_LOOT_PROBE_H
#include <stddef.h>
#include <stdint.h>

// Device experiment: collect bounded, owned label snapshots and sample them on
// an independent UI tick. The original game still draws and handles taps.
void dm_loot_probe_frame(const void *records,size_t bytes,unsigned count,
                         uint32_t viewport,float ui_dy,uint64_t ns);
void dm_loot_probe_reset(void);
void dm_loot_probe_tick(uint64_t ns);
void dm_loot_probe_stats(uint64_t *frames,uint64_t *invalid,uint64_t *ticks,
                         uint64_t *samples,uint64_t *last_count);
#endif
