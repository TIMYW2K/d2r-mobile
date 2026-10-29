#include "../dm_loot_exchange.h"
#include <assert.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MS UINT64_C(1000000)
static dm_loot_exchange bridge;
static _Atomic int done;
static void *writer(void *arg) {
    (void)arg;
    dm_loot_owned_snapshot *s=calloc(1,sizeof *s); assert(s);
    s->geometry.viewport=2; s->geometry.count=1;
    for (unsigned i=1;i<=5000;i++) {
        s->geometry.time_ns=1000*MS+(uint64_t)i*MS;
        s->geometry.labels[0]=(dm_loot_entry){i,{(float)i,0,20,10}};
        snprintf(s->labels[0].text,sizeof s->labels[0].text,"%u",i);
        assert(dm_loot_exchange_publish(&bridge,s));
    }
    free(s); atomic_store(&done,1); return NULL;
}
int main(void) {
    assert(dm_loot_exchange_init(&bridge));
    dm_loot_owned_snapshot *s=calloc(1,sizeof *s), *out=calloc(1,sizeof *out);
    dm_loot_pair *pair=calloc(1,sizeof *pair); assert(s && out && pair);
    s->geometry=(dm_loot_snapshot){.time_ns=100*MS,.viewport=1,.count=1,.labels={{5,{0,0,20,10}}}};
    strcpy(s->labels[0].text,"Rune");
    assert(dm_loot_exchange_publish(&bridge,s));
    s->geometry.time_ns=132*MS;s->geometry.labels[0].rect.x=32;
    assert(dm_loot_exchange_publish(&bridge,s));
    assert(dm_loot_exchange_copy(&bridge,pair,148*MS));
    assert(dm_loot_exchange_pair_valid(&bridge,pair));
    assert(dm_loot_pair_sample(pair,148*MS,out));
    assert(out->geometry.count==1 && out->geometry.labels[0].rect.x==16 && !strcmp(out->labels[0].text,"Rune"));
    assert(!dm_loot_exchange_copy(&bridge,pair,400*MS)); // stale
    dm_loot_exchange_invalidate(&bridge);
    assert(!dm_loot_exchange_pair_valid(&bridge,pair));
    assert(!dm_loot_exchange_copy(&bridge,pair,148*MS));
    memset(s,0,sizeof *s); s->geometry.time_ns=300*MS;s->geometry.viewport=4;
    assert(dm_loot_exchange_publish(&bridge,s)); // empty complete set
    assert(dm_loot_exchange_copy(&bridge,pair,301*MS));
    assert(dm_loot_pair_sample(pair,301*MS,out) && out->geometry.count==0);
    assert(dm_loot_exchange_pair_valid(&bridge,pair));
    s->geometry.time_ns=320*MS; s->geometry.viewport=5;
    assert(dm_loot_exchange_publish(&bridge,s));
    assert(!dm_loot_exchange_pair_valid(&bridge,pair)); // viewport changed
    dm_loot_exchange_invalidate(&bridge);
    pthread_t th;assert(!pthread_create(&th,NULL,writer,NULL));
    while (!atomic_load(&done)) {
        // Large clock tolerance is not used in the feature; pass the current
        // published timestamp through the synchronized copy below in this test.
        if (pthread_mutex_trylock(&bridge.lock)==0) {
            uint64_t ts=bridge.current.geometry.time_ns;
            pthread_mutex_unlock(&bridge.lock);
            if (ts && dm_loot_exchange_copy(&bridge,pair,ts+5*MS)) {
                assert(dm_loot_pair_sample(pair,ts+5*MS,out));
                assert(out->geometry.count==1);
                assert((unsigned)strtoul(out->labels[0].text,NULL,10)==out->geometry.labels[0].id);
            }
        }
    }
    assert(!pthread_join(th,NULL));
    dm_loot_exchange_invalidate(&bridge);
    assert(!dm_loot_exchange_copy(&bridge,pair,7000*MS));
    dm_loot_exchange_destroy(&bridge);free(s);free(out);free(pair);
    puts("PASS loot snapshot bridge: interpolation, 60 Hz frame plans, stale/scene fallback, concurrent handoff");
    return 0;
}
