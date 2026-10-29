#include "../dm_loot_interp.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#define MS UINT64_C(1000000)
static void near(float a,float b) { assert(fabsf(a-b)<0.001f); }
// A deterministic scene moving at 120 pixels/s, observed at the specified
// game rate and sampled by a 60 Hz display clock. Buffer exactly one game
// interval so the output time is always between two known samples.
static void different_rates(unsigned fps) {
    dm_loot_track track = {0};
    dm_loot_rect out;
    uint64_t period = UINT64_C(1000000000) / fps;
    uint64_t origin = UINT64_C(1000000000);
    unsigned next = 0, checked = 0;
    for (unsigned frame = 0; frame <= 180; frame++) {
        uint64_t display = origin + (uint64_t)frame * UINT64_C(1000000000) / 60;
        while (origin + (uint64_t)next * period <= display) {
            uint64_t ts = origin + (uint64_t)next * period;
            float x = (float)((double)(ts - origin) * 120.0 / 1e9);
            assert(dm_loot_track_push(&track, 77, 1, ts, (dm_loot_rect){x,0,90,20}, 0));
            next++;
        }
        if (display < origin + 2 * period) continue;
        uint64_t target = display - period;
        float expected = (float)((double)(target - origin) * 120.0 / 1e9);
        assert(dm_loot_track_sample(&track,target,&out));
        assert(fabsf(out.x-expected)<0.002f);
        checked++;
    }
    assert(checked > 100);
}
int main(void) {
    dm_loot_track s={0}; dm_loot_rect o;
    assert(!dm_loot_track_sample(&s, 1, &o));
    assert(dm_loot_track_push(&s, 42, 1, 100*MS, (dm_loot_rect){0,20,100,30}, 0));
    assert(dm_loot_track_push(&s, 42, 1, 132*MS, (dm_loot_rect){32,36,100,30}, 0));
    assert(dm_loot_track_sample(&s, 100*MS, &o)); near(o.x,0); near(o.y,20);
    assert(dm_loot_track_sample(&s, 116*MS, &o)); near(o.x,16); near(o.y,28);
    assert(dm_loot_track_sample(&s, 132*MS, &o)); near(o.x,32); near(o.y,36);
    assert(dm_loot_track_sample(&s, 150*MS, &o)); near(o.x,32); // no extrapolation
    assert(dm_loot_track_push(&s, 42, 1, 148*MS, (dm_loot_rect){48,44,120,40}, 0));
    assert(dm_loot_track_sample(&s, 140*MS, &o)); near(o.x,40); near(o.w,120); near(o.h,40);
    assert(dm_loot_track_push(&s, 42, 1, 164*MS, (dm_loot_rect){90,90,100,30}, 1));
    assert(dm_loot_track_sample(&s, 148*MS, &o)); near(o.x,90); // camera cut
    assert(dm_loot_track_push(&s, 42, 2, 196*MS, (dm_loot_rect){60,30,100,30}, 0));
    assert(dm_loot_track_sample(&s, 180*MS, &o)); near(o.x,60); // viewport
    assert(dm_loot_track_push(&s, 43, 2, 228*MS, (dm_loot_rect){8,3,100,30}, 0));
    assert(dm_loot_track_sample(&s, 212*MS, &o)); near(o.x,8); // different item
    assert(dm_loot_track_push(&s, 43, 2, 400*MS, (dm_loot_rect){9,3,100,30}, 0));
    assert(dm_loot_track_sample(&s, 300*MS, &o)); near(o.x,9); // stale
    assert(dm_loot_track_push(&s, 43, 2, 399*MS, (dm_loot_rect){7,3,100,30}, 0));
    assert(dm_loot_track_sample(&s, 399*MS, &o)); near(o.x,7); // timestamp rollback
    assert(!dm_loot_track_push(&s, 43, 2, 410*MS, (dm_loot_rect){NAN,3,100,30}, 0));
    assert(!dm_loot_track_push(&s, 43, 2, 410*MS, (dm_loot_rect){1,3,0,30}, 0));
    dm_loot_track_clear(&s);
    assert(!dm_loot_track_sample(&s, 420*MS, &o)); // removed immediately
    // Match by item ID rather than array position; removal is immediate and a
    // newly appearing label never travels from a different item's position.
    dm_loot_snapshot a={.time_ns=100*MS,.viewport=1,.count=3,
        .labels={{5,{0,0,20,10}},{6,{20,0,20,10}},{7,{30,0,20,10}}}};
    dm_loot_snapshot b={.time_ns=132*MS,.viewport=1,.count=3,
        .labels={{7,{62,0,24,12}},{5,{32,0,20,10}},{8,{100,0,20,10}}}};
    dm_loot_entry sampled[3]; unsigned n=99;
    assert(dm_loot_snapshot_sample(&a,&b,116*MS,0,sampled,3,&n) && n==3);
    assert(sampled[0].id==7 && sampled[1].id==5 && sampled[2].id==8);
    near(sampled[0].rect.x,46); near(sampled[0].rect.w,24); near(sampled[0].rect.h,12); near(sampled[1].rect.x,16); near(sampled[2].rect.x,100);
    assert(dm_loot_snapshot_sample(&a,&b,116*MS,1,sampled,3,&n)); near(sampled[0].rect.x,62);
    b.viewport=2;
    assert(dm_loot_snapshot_sample(&a,&b,116*MS,0,sampled,3,&n)); near(sampled[1].rect.x,32);
    b.viewport=1; n=99;
    assert(!dm_loot_snapshot_sample(&a,&b,116*MS,0,sampled,2,&n) && n==99);
    b.labels[2].id=5; n=99;
    assert(!dm_loot_snapshot_sample(&a,&b,116*MS,0,sampled,3,&n) && n==99);
    b.labels[2].id=8; b.count=0;
    assert(dm_loot_snapshot_sample(&a,&b,116*MS,0,sampled,3,&n) && n==0);
    const unsigned rates[] = {15,24,30,45,60,90,120};
    for (unsigned i=0; i<sizeof(rates)/sizeof(rates[0]); i++) different_rates(rates[i]);
    puts("PASS loot position interpolation: 15/24/30/45/60/90/120 Hz to 60 Hz, cuts, identity, stale, invalid, removal");
    return 0;
}
