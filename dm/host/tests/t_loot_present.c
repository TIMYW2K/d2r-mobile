#include "../dm_loot_present.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MS UINT64_C(1000000)
static void put(unsigned char *r,unsigned o,uint32_t n){memcpy(r+o,&n,4);}
int main(void) {
    dm_loot_present *p=calloc(1,sizeof *p);dm_loot_owned_snapshot *f=calloc(1,sizeof *f);
    assert(p&&f&&dm_loot_present_init(p));
    unsigned char a[DM_LOOT_RECORD_SIZE]={0},b[DM_LOOT_RECORD_SIZE]={0};
    put(a,0,10);put(a,4,20);put(a,8,50);put(a,12,20);put(a,16,3);
    memcpy(a+0x24,"Potion",7);put(a,0xac,255);put(a,0xb8,255);
    put(b,0,100);put(b,4,100);put(b,8,50);put(b,12,20);put(b,16,4);
    memcpy(b+0x24,"Rune",5);put(b,0xac,255);put(b,0xb8,255);
    unsigned char raw[2*DM_LOOT_RECORD_SIZE];memcpy(raw,a,sizeof a);memcpy(raw+sizeof a,b,sizeof b);
    assert(dm_loot_capture_owned(raw,sizeof raw,2,100*MS,1,0,f));
    unsigned char eligible[2]={1,0};
    assert(dm_loot_present_commit(p,f,eligible,7,101*MS));
    assert(dm_loot_present_can_hide(p,a,sizeof a,7,102*MS,0));
    assert(!dm_loot_present_can_hide(p,b,sizeof b,7,102*MS,0));
    assert(!dm_loot_present_can_hide(p,a,sizeof a,8,102*MS,0));
    assert(!dm_loot_present_can_hide(p,a,sizeof a,7,202*MS,0));
    uint32_t id=0;
    assert(dm_loot_present_hit(p,20,25,0,7,102*MS,&id) && id==3);
    assert(!dm_loot_present_hit(p,110,105,0,7,102*MS,&id));
    put(a,0,400);assert(!dm_loot_present_can_hide(p,a,sizeof a,7,102*MS,0));put(a,0,10);
    put(a,0xac,42);assert(!dm_loot_present_can_hide(p,a,sizeof a,7,102*MS,0));put(a,0xac,255);
    memcpy(a+0x24,"Poison",7);assert(!dm_loot_present_can_hide(p,a,sizeof a,7,102*MS,0));
    memcpy(a+0x24,"Potion",7);a[0x14]=1;assert(!dm_loot_present_can_hide(p,a,sizeof a,7,102*MS,0));a[0x14]=0;
    dm_loot_present_invalidate(p);
    assert(!dm_loot_present_can_hide(p,a,sizeof a,7,102*MS,0));
    assert(!dm_loot_present_hit(p,20,25,0,7,102*MS,&id));
    dm_loot_present_destroy(p);free(p);free(f);
    puts("PASS loot presentation handshake: eligible-only, style/text match, TTL, epoch, hit and invalidation");
    return 0;
}
