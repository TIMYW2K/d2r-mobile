#include "../dm_3d_scale_policy.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#define EQ(a,b) assert(fabsf((a)-(b))<.00001f)
int main(void) {
    EQ(dm_scale3d_target(.9f,0,0,0,0,0,0),.9f);
    EQ(dm_scale3d_target(.9f,.8f,0,0,0,0,0),.8f);
    EQ(dm_scale3d_target(.9f,.8f,.84f,0,0,0,0),.8f*.84f/.9f);
    EQ(dm_scale3d_target(.9f,.65f,0,.72f,0,0,0),.65f);
    EQ(dm_scale3d_target(.9f,.8f,0,.66f,0,0,0),.66f);
    EQ(dm_scale3d_target(.9f,.65f,.72f,.66f,0,0,0),.52f);
    EQ(dm_scale3d_target(.8f,.65f,0,0,1,0,0),.8f);
    EQ(dm_scale3d_target(1,.65f,.60f,.66f,0,1,0),1);
    EQ(dm_scale3d_target(.8f,.65f,.60f,.66f,1,1,0),.8f);
    EQ(dm_scale3d_target(.9f,0,.72f,.66f,0,0,0),.66f);
    EQ(dm_scale3d_target(.9f,.8f,.72f,.66f,0,0,1),1);
    EQ(dm_scale3d_target(.9f,.65f,.60f,.66f,0,0,0),.5f);
    EQ(dm_scale3d_target(.9f,NAN,0,0,0,0,0),.9f);
    EQ(dm_scale3d_target(.9f,.4f,0,0,0,0,0),.9f);
    puts("PASS MetalFX quality integration: independent thermal cap, relative dynamic tiers, forced native, environment override, ruler and fallback");
}
