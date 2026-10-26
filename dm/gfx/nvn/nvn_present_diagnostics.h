#pragma once
#include <stddef.h>
static inline int nvn_present_dropped(double interval,double period_ms) {
    if (interval<0) return 1;
    if (!(interval>0)) return 0;
    if (!(period_ms>0)) period_ms=1000.0/60;
    return interval>period_ms*1.5;
}
// Coordinates stay within the visible crop, never the square swapchain padding.
static inline int nvn_probe_points(unsigned width,unsigned height,const int crop[4],unsigned points[5][2]) {
    unsigned x=0,y=0,w=width,h=height;
    if (!w || !h) return 0;
    if (crop && (crop[2] || crop[3])) {
        if (crop[0]<0 || crop[1]<0 || crop[2]<=0 || crop[3]<=0 ||
            (unsigned)crop[0]>=width || (unsigned)crop[1]>=height ||
            (unsigned)crop[2]>width-(unsigned)crop[0] || (unsigned)crop[3]>height-(unsigned)crop[1]) return 0;
        x=crop[0];y=crop[1];w=crop[2];h=crop[3];
    }
    const unsigned numer[5][2]={{2,2},{1,1},{3,1},{1,3},{3,3}};
    for(int i=0;i<5;i++) { points[i][0]=x+w*numer[i][0]/4;points[i][1]=y+h*numer[i][1]/4; }
    return 1;
}

static inline double nvn_present_expected_period(double requested_ms,double target_hz) {
    double host=target_hz>0?1000.0/target_hz:1000.0/60;
    return requested_ms>host?requested_ms:host;
}
