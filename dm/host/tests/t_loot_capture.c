#include "../dm_loot_capture.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void put(unsigned char *p, unsigned off, int32_t v) { memcpy(p+off,&v,4); }
int main(void) {
    unsigned char raw[3*DM_LOOT_RECORD_SIZE]={0};
    unsigned char *a=raw, *b=raw+DM_LOOT_RECORD_SIZE, *c=b+DM_LOOT_RECORD_SIZE;
    put(a,0,10); put(a,4,20); put(a,8,80); put(a,12,24); put(a,16,42);
    memcpy(a+0x24,"Rune",5); a[0x144]=1;
    put(b,0,90); put(b,4,30); put(b,8,90); put(b,12,25); put(b,16,43);
    memcpy(b+0x24,"Potion",7);
    put(c,8,50); put(c,12,10); put(c,16,-1);
    static dm_loot_captured_snapshot out;
    assert(dm_loot_capture(raw,sizeof raw,3,1234,7,15,&out));
    assert(out.geometry.count==2 && out.geometry.time_ns==1234 && out.geometry.viewport==7);
    assert(out.geometry.labels[0].id==42 && out.geometry.labels[0].rect.y==35);
    assert(out.geometry.labels[1].id==43 && out.geometry.labels[1].rect.x==90);
    assert(!memcmp(out.record[0],a,DM_LOOT_RECORD_SIZE) && out.record[0][0x144]==1);
    memset(a,0,DM_LOOT_RECORD_SIZE); assert(!strcmp((char *)out.record[0]+0x24,"Rune"));
    assert(!dm_loot_capture(raw,sizeof raw-1,3,1,0,0,&out));
    assert(!dm_loot_capture(NULL,DM_LOOT_RECORD_SIZE,1,1,0,0,&out));
    assert(!dm_loot_capture(raw,sizeof raw,3,0,0,0,&out));
    assert(!dm_loot_capture(raw,sizeof raw,513,1,0,0,&out));
    put(a,8,1); put(a,12,1); put(a,16,43);
    assert(!dm_loot_capture(raw,sizeof raw,3,555,0,0,&out)); // duplicate
    assert(out.geometry.time_ns==1234 && out.geometry.count==2);
    assert(dm_loot_capture(NULL,0,0,666,9,0,&out));
    assert(out.geometry.count==0 && out.geometry.time_ns==666);
    static dm_loot_owned_snapshot owned;
    // The game can include U+E07E color codes and UTF-8 Chinese / multiline.
    memset(raw,0,sizeof raw);
    put(a,8,80); put(a,12,24); put(a,16,42);
    const char name[]="\xee\x81\xbe" "4" "\xe7\xac\xa6\xe6\x96\x87\nRune";
    memset(a+0x24,0x5a,128);
    memcpy(a+0x24,name,sizeof name);
    a[0x14]=0x12; a[0x23]=0x34;
    assert(dm_loot_capture_owned(raw,sizeof raw,1,700,1,5,&owned));
    assert(owned.geometry.count==1 && owned.geometry.labels[0].id==42);
    assert(owned.labels[0].style[0]==0x12 && owned.labels[0].style[15]==0x34);
    assert(!memcmp(owned.labels[0].text,a+0x24,sizeof name));
    for (size_t k=sizeof name;k<DM_LOOT_INLINE_TEXT;k++) assert(owned.labels[0].text[k]==0);
    memset(a+0x24,'x',128);
    assert(!dm_loot_capture_owned(raw,sizeof raw,1,800,1,5,&owned)); // no terminator
    assert(owned.geometry.time_ns==700);
    memset(a+0x24,0,128); a[0x24]=0xc0; a[0x25]=0x80;
    assert(!dm_loot_capture_owned(raw,sizeof raw,1,800,1,5,&owned)); // overlong
    memset(a+0x24,0,128); a[0x24]=0xed; a[0x25]=0xa0; a[0x26]=0x80;
    assert(!dm_loot_capture_owned(raw,sizeof raw,1,800,1,5,&owned)); // surrogate
    memset(a+0x24,0,128); a[0x24]=0xf4; a[0x25]=0x90; a[0x26]=0x80; a[0x27]=0x80;
    assert(!dm_loot_capture_owned(raw,sizeof raw,1,800,1,5,&owned)); // > U+10FFFF
    memset(a+0x24,0,128); a[0x24]=0xe4; a[0x25]=0xb8;
    assert(!dm_loot_capture_owned(raw,sizeof raw,1,800,1,5,&owned)); // incomplete
    assert(owned.geometry.time_ns==700);
    memset(a+0x24,0,128); a[0x24]=0xf4; a[0x25]=0x8f; a[0x26]=0xbf; a[0x27]=0xbf;
    assert(dm_loot_capture_owned(raw,sizeof raw,1,810,1,5,&owned)); // U+10FFFF
    memset(a+0x24,'A',127); a[0x24+127]=0;
    assert(dm_loot_capture_owned(raw,sizeof raw,1,820,1,5,&owned)); // full buffer
    assert(owned.labels[0].text[126]=='A' && owned.labels[0].text[127]==0);
    assert(dm_loot_capture_owned(NULL,0,0,900,1,0,&owned) && owned.geometry.count==0);
    puts("PASS loot capture: bounded original records, independent copies, geometry, empty and invalid frames");
    return 0;
}
