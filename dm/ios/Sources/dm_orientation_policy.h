#pragma once
// posture: -1 unknown/flat, 0 landscape, 1 portrait. Main-thread, bounded retry.
typedef struct { int posture, attempts; double next; } dm_orientation_retry;
static inline int dm_orientation_retry_step(dm_orientation_retry *s,int active,int posture,int settled,double now) {
    if (!active || posture<0 || settled) { *s=(dm_orientation_retry){-1,0,0}; return 0; }
    if (posture!=s->posture) *s=(dm_orientation_retry){posture,0,0};
    if (s->attempts>=6 || now<s->next) return 0;
    s->attempts++; s->next=now+.5; return 1;
}
