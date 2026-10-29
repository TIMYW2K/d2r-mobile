#include "dm_loot_exchange.h"
#include <string.h>

static int valid(const dm_loot_owned_snapshot *s) {
    uint16_t index[DM_LOOT_HASH_CAP];
    if (!s || !dm_loot_index(&s->geometry,index)) return 0;
    for (unsigned i=0;i<s->geometry.count;i++) {
        const unsigned char *p=(const unsigned char *)s->labels[i].text;
        const unsigned char *end=memchr(p,0,DM_LOOT_INLINE_TEXT);
        if (!end || end==p || !dm_loot_utf8_valid(p,(size_t)(end-p))) return 0;
    }
    return 1;
}
static int fresh(uint64_t ts,uint64_t now) {
    return ts && now >= ts && now-ts <= DM_LOOT_MAX_AGE_NS;
}
int dm_loot_exchange_init(dm_loot_exchange *x) {
    if (!x) return 0;
    memset(x,0,sizeof *x);
    atomic_init(&x->epoch,1);
    return pthread_mutex_init(&x->lock,NULL)==0;
}
void dm_loot_exchange_destroy(dm_loot_exchange *x) {
    if (x) pthread_mutex_destroy(&x->lock);
}
int dm_loot_exchange_publish(dm_loot_exchange *x,const dm_loot_owned_snapshot *s) {
    if (!x || !valid(s)) return 0;
    if (pthread_mutex_lock(&x->lock)) return 0;
    const dm_loot_snapshot *old=&x->current.geometry,*cur=&s->geometry;
    if (old->time_ns && old->viewport==cur->viewport && cur->time_ns>old->time_ns &&
        cur->time_ns-old->time_ns<=UINT64_C(100000000))
        x->previous=x->current;
    else {
        x->previous.geometry.time_ns=0;
        atomic_fetch_add_explicit(&x->epoch,1,memory_order_release);
    }
    x->current=*s;
    pthread_mutex_unlock(&x->lock);
    return 1;
}
void dm_loot_exchange_invalidate(dm_loot_exchange *x) {
    if (!x || pthread_mutex_lock(&x->lock)) return;
    x->previous.geometry.time_ns=0; x->current.geometry.time_ns=0;
    atomic_fetch_add_explicit(&x->epoch,1,memory_order_release);
    pthread_mutex_unlock(&x->lock);
}
int dm_loot_exchange_copy(dm_loot_exchange *x,dm_loot_pair *pair,uint64_t now) {
    if (!x || !pair || pthread_mutex_trylock(&x->lock)) return 0;
    int ok=fresh(x->current.geometry.time_ns,now);
    if (ok) {
        pair->epoch=atomic_load_explicit(&x->epoch,memory_order_relaxed);
        pair->previous=x->previous; pair->current=x->current;
    }
    pthread_mutex_unlock(&x->lock);
    return ok;
}
int dm_loot_exchange_pair_valid(const dm_loot_exchange *x,const dm_loot_pair *pair) {
    return x && pair && pair->epoch &&
        pair->epoch==atomic_load_explicit(&x->epoch,memory_order_acquire);
}
int dm_loot_pair_sample(const dm_loot_pair *pair,uint64_t now,dm_loot_owned_snapshot *out) {
    if (!pair || !out || !fresh(pair->current.geometry.time_ns,now)) return 0;
    const dm_loot_snapshot *prev=&pair->previous.geometry,*cur=&pair->current.geometry;
    uint64_t delay=0;
    if (prev->time_ns && prev->viewport==cur->viewport && cur->time_ns>prev->time_ns &&
        cur->time_ns-prev->time_ns<=UINT64_C(100000000))
        delay=cur->time_ns-prev->time_ns;
    uint64_t target=now>delay ? now-delay : 0;
    unsigned count=0;
    if (!dm_loot_snapshot_sample(prev,cur,target,0,out->geometry.labels,DM_LOOT_MAX_LABELS,&count)) return 0;
    for (unsigned i=0;i<count;i++) out->labels[i]=pair->current.labels[i];
    out->geometry.count=count; out->geometry.viewport=cur->viewport; out->geometry.time_ns=now;
    return 1;
}
