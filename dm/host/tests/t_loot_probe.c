#include "../dm_loot_probe.h"
#include "../dm_loot_capture.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#define MS UINT64_C(1000000)
static void put(unsigned char *p,unsigned off,int32_t v){memcpy(p+off,&v,4);}
int main(void) {
    unsigned char r[DM_LOOT_RECORD_SIZE]={0};
    put(r,0,10);put(r,4,20);put(r,8,70);put(r,12,24);put(r,16,5);
    memcpy(r+0x24,"Potion",7);
    dm_loot_probe_frame(r,sizeof r,1,7,5,100*MS);
    put(r,0,42);
    dm_loot_probe_frame(r,sizeof r,1,7,5,132*MS);
    dm_loot_probe_tick(148*MS);
    uint64_t f=0,b=0,t=0,s=0,n=0;
    dm_loot_probe_stats(&f,&b,&t,&s,&n);
    assert(f==2 && b==0 && t==1 && s==1 && n==1);
    memset(r+0x24,'A',128);
    dm_loot_probe_frame(r,sizeof r,1,7,5,164*MS);
    dm_loot_probe_tick(165*MS);
    dm_loot_probe_stats(&f,&b,&t,&s,&n);
    assert(f==3 && b==1 && t==2 && s==1);
    dm_loot_probe_frame(NULL,0,0,8,0,200*MS);
    dm_loot_probe_tick(201*MS);
    dm_loot_probe_stats(&f,&b,&t,&s,&n);
    assert(f==4 && b==1 && t==3 && s==2 && n==0);
    dm_loot_probe_reset();dm_loot_probe_tick(202*MS);
    dm_loot_probe_stats(&f,&b,&t,&s,&n);assert(t==4 && s==2);
    puts("PASS live loot probe: capture, sampling, rejection, empty set and reset");
    return 0;
}
