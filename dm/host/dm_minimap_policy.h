#pragma once
#include <stdint.h>
// Per-scene bounded recovery. Once visible, a later user close is respected for that scene.
typedef struct { int active, area, pending, attempts; uint64_t next; } dm_minimap_entry;
static inline int dm_minimap_entry_step(dm_minimap_entry *s, uint64_t frame, int ready, int area, int shown) {
    if (!ready || area <= 0) { s->active = 0; return 0; }
    if (!s->active || area != s->area) {
        s->active = 1; s->area = area; s->pending = 1; s->attempts = 0; s->next = frame + 6;
    }
    if (!s->pending) return 0;
    if (shown) { s->pending = 0; return 0; }
    if (frame < s->next) return 0;
    if (s->attempts >= 3) { s->pending = 0; return 0; }
    ++s->attempts; s->next = frame + 30;
    return 1;
}
