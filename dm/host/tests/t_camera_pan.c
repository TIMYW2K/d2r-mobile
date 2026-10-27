#include "../dm_camera_pan.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
int main(void) {
    float dx,dy;dm_camera_pan_delta(0,1,&dx,&dy);assert(dx==0 && dy==0);
    dm_camera_pan_delta(1,0,&dx,&dy);assert(dx==0 && dy==0);
    dm_camera_pan_delta(1,1,&dx,&dy);assert(dx==5 && dy==15);
    float m[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
    dm_camera_pan_projection(m,1206,2622,dx,dy,1288);
    // After the game's screen projection and UI composition, all world points
    // move exactly right5/down15, independent of zoom or UI band's height.
    assert(fabsf(m[12]*1206/2-5)<.001f);
    assert(fabsf(-m[13]*2622/2+1288-15)<.001f);
    float old=m[12];dm_camera_pan_projection(m,0,2622,dx,dy,1288);assert(m[12]==old);
    puts("camera pan: landscape/menu identity, portrait shift and UI projection aligned");return 0;
}
