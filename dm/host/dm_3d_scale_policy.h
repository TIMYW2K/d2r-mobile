#pragma once
// MetalFX controls only the 3D baseline. Dynamic degradation is relative to
// that baseline; the existing thermal limits remain independent absolute caps.
static inline float dm_scale3d_target(float configured,float hint,float dynamic_tier,
        float thermal_cap,int env_override,int forced,int ruler_truth) {
    int fx=!env_override && !forced && hint>=.5f && hint<=1.0f;
    float cfg=fx?hint:configured;
    float value=forced || dynamic_tier<=0 ? cfg : dynamic_tier*(fx?cfg/configured:1.0f);
    if (value>cfg) value=cfg;
    if (!forced && thermal_cap>0 && value>thermal_cap) value=thermal_cap;
    if (ruler_truth) value=1.0f;
    if (!(value>=.5f)) value=.5f;
    return value>1.0f?1.0f:value;
}
