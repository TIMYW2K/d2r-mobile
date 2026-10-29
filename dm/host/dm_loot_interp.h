#ifndef DM_LOOT_INTERP_H
#define DM_LOOT_INTERP_H

// Experimental geometry only. The game's labels are baked into the rendered
// frame; this helper does not hide or draw them, and does not alter hit tests.
// Samples belong to the same item and viewport generation. The caller must
// discard the track immediately when an item disappears.
#include <stdint.h>
#include <math.h>

typedef struct { float x, y, w, h; } dm_loot_rect;
typedef struct {
    uint32_t item_id, viewport;
    uint64_t previous_ns, current_ns;
    dm_loot_rect previous, current;
    int valid, paired;
} dm_loot_track;

static inline int dm_loot_rect_valid(dm_loot_rect r) {
    return isfinite(r.x) && isfinite(r.y) && isfinite(r.w) &&
        isfinite(r.h) && r.w > 0 && r.h > 0;
}

static inline void dm_loot_track_clear(dm_loot_track *s) {
    *s = (dm_loot_track){0};
}

// A camera cut, a layout/viewport generation change, a recycled item ID,
// nonmonotonic timestamps or a long gap snap to the new sample.
static inline int dm_loot_track_push(dm_loot_track *s, uint32_t item_id,
    uint32_t viewport, uint64_t time_ns, dm_loot_rect r, int camera_cut) {
    if (!s || !time_ns || !dm_loot_rect_valid(r)) return 0;
    int pair = s->valid && !camera_cut && s->item_id == item_id &&
        s->viewport == viewport && time_ns > s->current_ns &&
        time_ns - s->current_ns <= UINT64_C(100000000);
    if (pair) {
        s->previous = s->current;
        s->previous_ns = s->current_ns;
    } else {
        s->previous = r;
        s->previous_ns = time_ns;
    }
    s->current = r;
    s->current_ns = time_ns;
    s->item_id = item_id;
    s->viewport = viewport;
    s->valid = 1;
    s->paired = pair;
    return 1;
}

// Delayed interpolation: caller supplies a time in the sample timeline,
// normally display time minus a fixed one-sample buffer. No extrapolation.
static inline int dm_loot_track_sample(const dm_loot_track *s,
    uint64_t time_ns, dm_loot_rect *out) {
    if (!s || !s->valid || !out) return 0;
    if (!s->paired || time_ns >= s->current_ns) { *out = s->current; return 1; }
    if (time_ns <= s->previous_ns) { *out = s->previous; out->w = s->current.w; out->h = s->current.h; return 1; }
    float a = (float)((double)(time_ns - s->previous_ns) /
                      (double)(s->current_ns - s->previous_ns));
    out->x = s->previous.x + a * (s->current.x - s->previous.x);
    out->y = s->previous.y + a * (s->current.y - s->previous.y);
    // Dimensions belong to the current text/layout: interpolating them can
    // stretch glyphs and make the displayed hit box disagree with the label.
    out->w = s->current.w;
    out->h = s->current.h;
    return 1;
}

// Immutable snapshots are produced by the game thread; handoff to a renderer
// must make a synchronized copy, never expose the game's mutable record array.
// A snapshot represents the COMPLETE visible set, not a partial update.
#define DM_LOOT_MAX_LABELS 512
#define DM_LOOT_HASH_CAP 1024
typedef struct { uint32_t id; dm_loot_rect rect; } dm_loot_entry;
typedef struct {
    uint64_t time_ns;
    uint32_t viewport;
    unsigned count;
    dm_loot_entry labels[DM_LOOT_MAX_LABELS];
} dm_loot_snapshot;

static inline unsigned dm_loot_hash(uint32_t id) {
    return (id * UINT32_C(2654435761)) & (DM_LOOT_HASH_CAP - 1);
}

// A zero index means empty; populated indices are 1-based, allowing ID zero.
static inline int dm_loot_index(const dm_loot_snapshot *s, uint16_t *index) {
    if (!s || !s->time_ns || s->count > DM_LOOT_MAX_LABELS) return 0;
    for (unsigned i = 0; i < DM_LOOT_HASH_CAP; i++) index[i] = 0;
    for (unsigned i = 0; i < s->count; i++) {
        if (s->labels[i].id == UINT32_MAX || !dm_loot_rect_valid(s->labels[i].rect)) return 0;
        unsigned h = dm_loot_hash(s->labels[i].id);
        while (index[h]) {
            if (s->labels[index[h] - 1].id == s->labels[i].id) return 0;
            h = (h + 1) & (DM_LOOT_HASH_CAP - 1);
        }
        index[h] = (uint16_t)(i + 1);
    }
    return 1;
}

// Sample the latest visible labels. The same returned rectangle must be used
// for painting AND hit testing; names, styling and rendering are not handled.
// An empty/invalid/unrelated previous snapshot snaps the entire current set.
// This function doesn't invent intermediate screen presents or guarantee 60 Hz.
static inline int dm_loot_snapshot_sample(const dm_loot_snapshot *previous,
    const dm_loot_snapshot *current, uint64_t sample_ns, int camera_cut,
    dm_loot_entry *out, unsigned cap, unsigned *count) {
    uint16_t cur_index[DM_LOOT_HASH_CAP], prev_index[DM_LOOT_HASH_CAP];
    if (!out || !count || !dm_loot_index(current, cur_index) || cap < current->count) return 0;
    int paired = previous && !camera_cut && previous->viewport == current->viewport &&
        previous->time_ns < current->time_ns &&
        current->time_ns - previous->time_ns <= UINT64_C(100000000) &&
        dm_loot_index(previous, prev_index);
    float alpha = 1;
    if (paired && sample_ns <= previous->time_ns) alpha = 0;
    else if (paired && sample_ns < current->time_ns)
        alpha = (float)((double)(sample_ns - previous->time_ns) /
                        (double)(current->time_ns - previous->time_ns));
    for (unsigned i = 0; i < current->count; i++) {
        out[i] = current->labels[i];
        if (!paired) continue;
        unsigned h = dm_loot_hash(out[i].id);
        while (prev_index[h] && previous->labels[prev_index[h] - 1].id != out[i].id)
            h = (h + 1) & (DM_LOOT_HASH_CAP - 1);
        if (!prev_index[h]) continue; // newly visible: snap
        dm_loot_rect a = previous->labels[prev_index[h] - 1].rect, b = out[i].rect;
        float dx=b.x-a.x, dy=b.y-a.y;
        // A jump is more likely a teleport / relayout than smooth motion.
        if (dx*dx+dy*dy > 96.0f*96.0f) continue;
        out[i].rect = (dm_loot_rect){a.x + alpha*(b.x-a.x), a.y + alpha*(b.y-a.y),
                                    b.w, b.h};
    }
    *count = current->count;
    return 1;
}
#endif
