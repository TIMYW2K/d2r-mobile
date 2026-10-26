#pragma once
static inline int dm_touch_end_recover(int occupied,int pending,int ended,int cancelled) {
    return occupied && !pending && (ended || cancelled);
}
static inline double dm_touch_end_time(double timestamp,double started,double now) {
    return timestamp>=started && timestamp<=now ? timestamp : now;
}

static inline int dm_touch_end_defer(int pending,int replay,int cancelled) { return pending && !replay && !cancelled; }
