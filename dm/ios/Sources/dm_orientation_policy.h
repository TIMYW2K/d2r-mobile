#pragma once
// posture: -1 unknown/flat, 0 landscape, 1 portrait. Main-thread, bounded retry.
typedef struct { int posture, attempts; double next; } dm_orientation_retry;
static inline int dm_orientation_retry_step(dm_orientation_retry *s,int active,int posture,int settled,double now) {
    if (!active || posture<0 || settled) { *s=(dm_orientation_retry){-1,0,0}; return 0; }
    if (posture!=s->posture) *s=(dm_orientation_retry){posture,0,0};
    if (s->attempts>=6 || now<s->next) return 0;
    s->attempts++; s->next=now+.5; return 1;
}

// Permit a landscape entry immediately: UIKit can then deliver the first
// landscape->portrait transition without depending on an out-of-run tick.
// If physically portrait on entry, retain landscape until a real posture
// change; unknown/flat readings cannot select portrait. No permanent wait gate.
typedef struct { int started, posture, enabled, reset_pending; } dm_orientation_entry;
static inline void dm_orientation_entry_observe(dm_orientation_entry *s,int ingame,int posture) {
    if (!ingame) {
        // Clear any late old-run sample once the front end is observed, but
        // never wait for that sample before permitting a new landscape run.
        if(s->reset_pending) *s=(dm_orientation_entry){0,-1,0,0};
        return;
    }
    if(posture<0) return;
    if (!s->started) {
        *s=(dm_orientation_entry){1,posture,posture==0,s->reset_pending};
        return;
    }
    if (posture==0 || (s->posture>=0 && posture!=s->posture)) s->enabled=1;
    s->posture=posture;
}
static inline void dm_orientation_entry_reset(dm_orientation_entry *s) {
    *s=(dm_orientation_entry){0,-1,0,1};
}
