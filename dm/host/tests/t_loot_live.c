#include "../dm_loot_probe.h"
#include "../dm_loot_live.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MS UINT64_C(1000000)
static int ingame=1,panel=0;
int dm_uiscreen_ingame(void){return ingame;}
int dm_uiscreen_panel(void){return panel;}
int dm_ui_proj_dy(void){return 5;}
int dm_focus_now(void){return 1;}
static void put(unsigned char *p,unsigned off,int32_t v){memcpy(p+off,&v,4);}
int main(void) {
    unsigned char r[DM_LOOT_RECORD_SIZE]={0};
    put(r,0,10);put(r,4,20);put(r,8,70);put(r,12,24);put(r,16,5);
    memcpy(r+0x24,"Potion",7);put(r,0xac,255);put(r,0xb8,255);
    dm_loot_probe_frame(r,sizeof r,1,7,5,100*MS);
    put(r,0,42);
    dm_loot_probe_frame(r,sizeof r,1,7,5,132*MS);
    dm_loot_probe_tick(148*MS);
    dm_loot_owned_snapshot *f=calloc(1,sizeof *f);assert(f);
    uint64_t epoch=0;assert(dm_loot_probe_plan(f,&epoch));
    unsigned char eligible[DM_LOOT_MAX_LABELS]={1};
    assert(!dm_loot_live_hide(r,148*MS));
    assert(dm_loot_live_commit(f,eligible,epoch,148*MS));
    assert(dm_loot_live_hide(r,149*MS));
    uint32_t id=0;
    assert(dm_loot_live_hit(40,30,0,149*MS,&id) && id==5);
    panel=1;assert(!dm_loot_live_hide(r,149*MS) && !dm_loot_live_hit(40,30,0,149*MS,&id));
    panel=0;put(r,0xac,1);assert(!dm_loot_live_hide(r,149*MS));put(r,0xac,255);
    assert(!dm_loot_live_hide(r,250*MS)); // stale acknowledgment
    dm_loot_probe_reset();assert(!dm_loot_live_hide(r,149*MS));
    dm_loot_live_invalidate();free(f);
    puts("PASS loot live bridge: opt-in presentation, native fallback, same-ID hit, panel, stale and reset");
    return 0;
}
