#pragma once
// posture: -1 unknown/flat, 0 landscape, 1 portrait. Main-thread, bounded retry.
typedef struct { int posture, attempts; double next; } dm_orientation_retry;
static inline int dm_orientation_retry_step(dm_orientation_retry *s,int active,int posture,int settled,double now) {
    if (!active || posture<0 || settled) { *s=(dm_orientation_retry){-1,0,0}; return 0; }
    if (posture!=s->posture) *s=(dm_orientation_retry){posture,0,0};
    if (s->attempts>=6 || now<s->next) return 0;
    s->attempts++; s->next=now+.5; return 1;
}

// A landscape front end must not rotate simply because entering a run widens
// UIKit's supported mask. Capture that run's initial posture, then require a
// subsequent physical change. Unknown/flat readings do not arm the gate.
typedef struct { int started, posture, enabled, wait_frontend; } dm_orientation_entry;
static inline void dm_orientation_entry_observe(dm_orientation_entry *s,int ingame,int posture) {
    if (s->wait_frontend) {
        if (!ingame) s->wait_frontend=0;
        return;
    }
    if (!s->started) {
        if (ingame) *s=(dm_orientation_entry){1,posture,0};
        return;
    }
    if (posture<0) return;
    if (s->posture>=0 && posture!=s->posture) s->enabled=1;
    s->posture=posture;
}
static inline void dm_orientation_entry_reset(dm_orientation_entry *s) {
    *s=(dm_orientation_entry){0,-1,0,1};
}
