#pragma once
#include <math.h>
// Request retries are bounded bursts, never a permanent lock after six failures.
typedef struct { int posture, attempts; double next; } dm_orientation_retry;
static inline int dm_orientation_retry_step(dm_orientation_retry *s,int active,int posture,int settled,double now) {
    if(!active || posture<0 || settled) { *s=(dm_orientation_retry){-1,0,0};return 0; }
    if(posture!=s->posture) *s=(dm_orientation_retry){posture,0,0};
    if(now<s->next) return 0;
    if(s->attempts>=6) s->attempts=0;
    s->attempts++;s->next=now+(s->attempts==6?2.0:.5);return 1;
}
// Physical posture is sampled independently of UIKit's allowed orientation.
// Exclude flat, upside-down and diagonal readings; require a stable 250ms.
static inline int dm_orientation_gravity(double x,double y,double z) {
    if(!isfinite(x)||!isfinite(y)||!isfinite(z) || fabs(z)>.94 || hypot(x,y)<.35) return -1;
    if(y<-.30 && fabs(y)>fabs(x)*1.25) return 1;
    if(fabs(x)>.30 && fabs(x)>fabs(y)*1.25) return 0;
    return -1;
}
typedef struct { int session, target, baseline, candidate; double since; unsigned changes; } dm_orientation_state;
static inline void dm_orientation_start(dm_orientation_state *s) { *s=(dm_orientation_state){1,0,-1,-1,0,0}; }
static inline void dm_orientation_stop(dm_orientation_state *s) { *s=(dm_orientation_state){0,0,-1,-1,0,0}; }
static inline int dm_orientation_sample(dm_orientation_state *s,int posture,double now) {
    if(!s->session) return 0;
    if(posture<0) { s->candidate=-1;return 0; }
    if(posture!=s->candidate) { s->candidate=posture;s->since=now;return 0; }
    if(now-s->since<.25) return 0;
    if(s->baseline<0) { s->baseline=posture;return 0; } // Entry always stays landscape.
    if(posture==s->baseline) return 0;
    s->baseline=posture;s->target=posture;s->changes++;return 1;
}
static inline int dm_orientation_target(const dm_orientation_state *s) { return s->session?s->target:0; }
