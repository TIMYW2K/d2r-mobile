#include "../dm_loot_draw.h"
#include <assert.h>
#include <stdio.h>
static int calls; static uint64_t a,b,c;
static void draw(uint64_t x,uint64_t y,uint64_t z) { calls++; a=x;b=y;c=z; }
int main(void) {
    uint64_t r[4]={11,22,33,44};
    dm_loot_native_forward(draw,r);
    assert(calls==1 && a==11 && b==22 && c==33 && r[0]==11 && r[3]==44);
    puts("PASS loot draw seam: native draw forwarded exactly once and arguments unchanged");
    return 0;
}
