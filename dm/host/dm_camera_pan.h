#pragma once
// Positive screen-space delta moves the scene right/down (camera left/up).
static inline void dm_camera_pan_delta(int portrait,int ingame,float *dx,float *dy) {
    *dx=portrait && ingame ? 5.0f : 0.0f;
    *dy=portrait && ingame ? 15.0f : 0.0f;
}
static inline void dm_camera_pan_projection(float *m,float width,float height,float dx,float dy,float ui_y) {
    if (!(width>0 && height>0)) return;
    for(int j=0;j<4;j++) {
        m[j*4]+=2.0f*dx/width*m[j*4+3];
        m[j*4+1]+=2.0f*(ui_y-dy)/height*m[j*4+3];
    }
}
