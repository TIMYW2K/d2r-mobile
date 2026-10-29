#ifndef DM_LOOT_CAPTURE_H
#define DM_LOOT_CAPTURE_H
#include "dm_loot_interp.h"
#include <stddef.h>
#include <string.h>

// Observed on the prepared 3.3 game image. These are copied as opaque bytes:
// interpreting them, drawing them or suppressing the game's draw is a separate
// feature and MUST be guarded by image-version and device validation.
#define DM_LOOT_RECORD_SIZE 0x148u
typedef struct {
    dm_loot_snapshot geometry;
    unsigned char record[DM_LOOT_MAX_LABELS][DM_LOOT_RECORD_SIZE];
} dm_loot_captured_snapshot;

static inline int32_t dm_loot_i32(const unsigned char *p) {
    int32_t v; memcpy(&v,p,sizeof v); return v;
}

// Called only on the game thread while its array is stable. Copies a complete
// set with matching geometry/metadata order. Caller must synchronize handoff.
// On failure the destination is untouched, so callers can preserve or hide
// the previous frame instead of presenting a partial set. Invalid/dead labels
// are skipped exactly as in dm_world.c; no names are logged.
static inline int dm_loot_capture(const void *records, size_t bytes,
    unsigned count, uint64_t ns, uint32_t viewport, float ui_dy,
    dm_loot_captured_snapshot *dst) {
    const unsigned char *p = (const unsigned char *)records;
    uint32_t ids[DM_LOOT_HASH_CAP] = {0}; unsigned char used[DM_LOOT_HASH_CAP] = {0};
    if (!dst || !ns || !isfinite(ui_dy) || count > DM_LOOT_MAX_LABELS ||
        bytes < (size_t)count * DM_LOOT_RECORD_SIZE || (count && !p)) return 0;
    // Validate the complete set before modifying the published destination.
    for (unsigned i = 0; i < count; i++) {
        const unsigned char *r = p + (size_t)i * DM_LOOT_RECORD_SIZE;
        int32_t w = dm_loot_i32(r+8), h = dm_loot_i32(r+12), id = dm_loot_i32(r+16);
        if (w <= 0 || h <= 0 || id == -1) continue;
        unsigned hsh = dm_loot_hash((uint32_t)id);
        while (used[hsh]) {
            if (ids[hsh] == (uint32_t)id) return 0;
            hsh = (hsh + 1) & (DM_LOOT_HASH_CAP - 1);
        }
        used[hsh] = 1; ids[hsh] = (uint32_t)id;
    }
    dst->geometry.time_ns = ns;
    dst->geometry.viewport = viewport;
    dst->geometry.count = 0;
    for (unsigned i = 0; i < count; i++) {
        const unsigned char *r = p + (size_t)i * DM_LOOT_RECORD_SIZE;
        int32_t w = dm_loot_i32(r+8), h = dm_loot_i32(r+12), id = dm_loot_i32(r+16);
        if (w <= 0 || h <= 0 || id == -1) continue;
        unsigned j = dst->geometry.count++;
        dst->geometry.labels[j].id = (uint32_t)id;
        dst->geometry.labels[j].rect = (dm_loot_rect){
            (float)dm_loot_i32(r), (float)dm_loot_i32(r+4) + ui_dy, (float)w, (float)h};
        memcpy(dst->record[j],r,DM_LOOT_RECORD_SIZE);
    }
    return 1;
}

#define DM_LOOT_INLINE_TEXT 128u
#define DM_LOOT_STYLE_BYTES 16u
typedef struct {
    char text[DM_LOOT_INLINE_TEXT];
    unsigned char style[DM_LOOT_STYLE_BYTES]; // background, still opaque
    uint32_t color[4]; // observed four 0..255 components, record +0xac
} dm_loot_owned_label;
typedef struct {
    dm_loot_snapshot geometry;
    dm_loot_owned_label labels[DM_LOOT_MAX_LABELS];
} dm_loot_owned_snapshot;

// Strict UTF-8 validator, preserving color codes/newlines byte-for-byte.
// Invalid/truncated input makes the future replacement renderer fall back to
// native drawing; it must never repair or silently truncate an item name.
static inline int dm_loot_utf8_valid(const unsigned char *s, size_t n) {
    for (size_t i=0; i<n;) {
        unsigned c=s[i++];
        if (c < 0x80) continue;
        unsigned need; unsigned lo=0x80, hi=0xbf;
        if (c >= 0xc2 && c <= 0xdf) need=1;
        else if (c >= 0xe0 && c <= 0xef) {
            need=2;
            if (c == 0xe0) lo=0xa0;
            if (c == 0xed) hi=0x9f;
        } else if (c >= 0xf0 && c <= 0xf4) {
            need=3;
            if (c == 0xf0) lo=0x90;
            if (c == 0xf4) hi=0x8f;
        } else return 0;
        if (n-i < need || s[i] < lo || s[i] > hi) return 0;
        i++;
        while (--need) { if (s[i] < 0x80 || s[i] > 0xbf) return 0; i++; }
    }
    return 1;
}

// Pure prototype: record+0x24 is passed to the game's builder with capacity
// 0x80 on the 3.3 image, and record+0x14 is passed as the background style.
// The latter is copied as opaque data, NOT treated as a pointer/color value.
// Validate the image contract and verify text/style on device before use.
// No failure may change dst; the native renderer must remain visible.
static inline int dm_loot_capture_owned(const void *records, size_t bytes,
    unsigned count, uint64_t ns, uint32_t viewport, float ui_dy,
    dm_loot_owned_snapshot *dst) {
    const unsigned char *p = (const unsigned char *)records;
    uint32_t ids[DM_LOOT_HASH_CAP] = {0}; unsigned char used[DM_LOOT_HASH_CAP] = {0};
    if (!dst || !ns || !isfinite(ui_dy) || count > DM_LOOT_MAX_LABELS ||
        bytes < (size_t)count * DM_LOOT_RECORD_SIZE || (count && !p)) return 0;
    for (unsigned i=0; i<count; i++) {
        const unsigned char *r=p+(size_t)i*DM_LOOT_RECORD_SIZE;
        int32_t w=dm_loot_i32(r+8),h=dm_loot_i32(r+12),id=dm_loot_i32(r+16);
        if (w<=0 || h<=0 || id == -1) continue;
        const unsigned char *str=r+0x24;
        const unsigned char *end=memchr(str,0,DM_LOOT_INLINE_TEXT);
        if (!end || end==str || !dm_loot_utf8_valid(str,(size_t)(end-str))) return 0;
        // The label drawer stores the four components returned by c741a0 at
        // +0xac. Out-of-range values are unverified: fall back to native draw.
        for (unsigned k=0;k<4;k++) {
            uint32_t component; memcpy(&component,r+0xac+4*k,4);
            if (component>255) return 0;
        }
        unsigned slot=dm_loot_hash((uint32_t)id);
        while (used[slot]) {
            if (ids[slot] == (uint32_t)id) return 0;
            slot=(slot+1)&(DM_LOOT_HASH_CAP-1);
        }
        used[slot]=1; ids[slot]=(uint32_t)id;
    }
    dst->geometry.time_ns=ns; dst->geometry.viewport=viewport; dst->geometry.count=0;
    for (unsigned i=0; i<count; i++) {
        const unsigned char *r=p+(size_t)i*DM_LOOT_RECORD_SIZE;
        int32_t w=dm_loot_i32(r+8),h=dm_loot_i32(r+12),id=dm_loot_i32(r+16);
        if (w<=0 || h<=0 || id == -1) continue;
        unsigned j=dst->geometry.count++;
        dst->geometry.labels[j].id=(uint32_t)id;
        dst->geometry.labels[j].rect=(dm_loot_rect){
            (float)dm_loot_i32(r),(float)dm_loot_i32(r+4)+ui_dy,(float)w,(float)h};
        const unsigned char *str=r+0x24;
        size_t text_len=(size_t)((const unsigned char *)memchr(str,0,DM_LOOT_INLINE_TEXT)-str);
        memcpy(dst->labels[j].text,str,text_len+1);
        memset(dst->labels[j].text+text_len+1,0,DM_LOOT_INLINE_TEXT-text_len-1);
        memcpy(dst->labels[j].style,r+0x14,DM_LOOT_STYLE_BYTES);
        memcpy(dst->labels[j].color,r+0xac,sizeof dst->labels[j].color);
    }
    return 1;
}
#endif
