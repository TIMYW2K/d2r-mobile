// Translate only the completed 3D image, before the game's HUD composition.
// Both the original upsample and MetalFX land here; geometry, shadows and depth
// keep their original matrices. Clamp-to-edge sampling fills the exposed edges.
extern void dm_cam_pan_pixels(float *,float *) __attribute__((weak));
static id<MTLTexture> g_scene_pan_copy;
static id<MTLBuffer> g_scene_pan_uv;
static id<MTLRenderPipelineState> g_scene_pan_pso;
static id<MTLSamplerState> g_scene_pan_sampler;
#if !DM_NVN_M3
static id<MTL4ArgumentTable> g_scene_pan_args;
#endif
static MTLPixelFormat g_scene_pan_format;
static float g_scene_pan_offset[2];
static int scene_pan_prepare(id<MTLTexture> target,float ux,float uy) {
    if (!g_scene_pan_pso || target.pixelFormat!=g_scene_pan_format) {
        NSString *source=@"#include <metal_stdlib>\nusing namespace metal;\n"
          "struct V { float4 p [[position]]; float2 uv; };\n"
          "vertex V vs(uint i [[vertex_id]]) { float2 uv=float2((i<<1)&2,i&2); V v; v.p=float4(uv*float2(2,-2)+float2(-1,1),0,1); v.uv=uv; return v; }\n"
          "fragment float4 fs(V v [[stage_in]], texture2d<float> t [[texture(0)]], sampler s [[sampler(0)]], constant float2& pan [[buffer(0)]]) { return t.sample(s,v.uv-pan); }\n";
        NSError *error=nil;id<MTLLibrary> lib=[g_dev newLibraryWithSource:source options:nil error:&error];
        if (!lib) { MLOG("Scene pan shader unavailable: %s",error.localizedDescription.UTF8String);return 0; }
        MTLRenderPipelineDescriptor *pd=[MTLRenderPipelineDescriptor new];
        pd.vertexFunction=[lib newFunctionWithName:@"vs"];pd.fragmentFunction=[lib newFunctionWithName:@"fs"];
        pd.colorAttachments[0].pixelFormat=target.pixelFormat;
        id<MTLRenderPipelineState> pso=[g_dev newRenderPipelineStateWithDescriptor:pd error:&error];
        if (!pso) return 0;
        if(g_scene_pan_pso) defer_obj_unref(g_scene_pan_pso);
        g_scene_pan_pso=pso;g_scene_pan_format=target.pixelFormat;
    }
    if (!g_scene_pan_sampler) {
        MTLSamplerDescriptor *sd=[MTLSamplerDescriptor new];sd.minFilter=sd.magFilter=MTLSamplerMinMagFilterLinear;
        sd.sAddressMode=sd.tAddressMode=MTLSamplerAddressModeClampToEdge;sd.supportArgumentBuffers=YES;
        g_scene_pan_sampler=[g_dev newSamplerStateWithDescriptor:sd];
    }
#if !DM_NVN_M3
    if(!g_scene_pan_args) {
        MTL4ArgumentTableDescriptor *ad=[MTL4ArgumentTableDescriptor new];
        ad.maxTextureBindCount=1;ad.maxSamplerStateBindCount=1;ad.maxBufferBindCount=1;
        g_scene_pan_args=[g_dev newArgumentTableWithDescriptor:ad error:nil];
    }
    if(!g_scene_pan_args) return 0;
#endif
    if(!g_scene_pan_copy || g_scene_pan_copy.width!=target.width || g_scene_pan_copy.height!=target.height || g_scene_pan_copy.pixelFormat!=target.pixelFormat) {
        MTLTextureDescriptor *td=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:target.pixelFormat width:target.width height:target.height mipmapped:NO];
        td.storageMode=MTLStorageModePrivate;td.usage=MTLTextureUsageShaderRead;
        id<MTLTexture> copy=[g_dev newTextureWithDescriptor:td];if(!copy) return 0;
        copy.label=@"Portrait scene pan copy";
        if(g_scene_pan_copy) defer_obj_release(g_scene_pan_copy);
        g_scene_pan_copy=copy;res_add(copy);
    }
    if(!g_scene_pan_uv || ux!=g_scene_pan_offset[0] || uy!=g_scene_pan_offset[1]) {
        float values[2]={ux,uy};
        // Constants are immutable while in flight, including orientation changes.
        id<MTLBuffer> uv=[g_dev newBufferWithBytes:values length:sizeof values options:MTLResourceStorageModeShared];
        if(!uv) return 0;
        if(g_scene_pan_uv) defer_obj_release(g_scene_pan_uv);
        g_scene_pan_uv=uv;res_add(uv);g_scene_pan_offset[0]=ux;g_scene_pan_offset[1]=uy;
    }
    return g_scene_pan_sampler!=nil;
}
static void scene_pan_after_draw(NvProg *pr,const p_draw *dw,const p_draw_idx *di) {
    if(!dm_cam_pan_pixels || !dm_screen_real || !pr || !pr->sh[1] ||
       strncmp(pr->sh[1]->key,"52aac1f2306b8fc5",16) || g_defer.active || g_defer_replaying ||
       !R.has_rt || R.rt.n!=1 || !R.rt.color[0] ||
       (dw && !dw->count) || (di && !di->count)) return;
    float dx,dy;dm_cam_pan_pixels(&dx,&dy);if(!dx && !dy) return;
    int W=0,H=0;if(!dm_screen_real(&W,&H) || W<=0 || H<=W) return;
    uint64_t serial;int level,slice;rt_sub(&R.rt,0,&serial,&level,&slice);
    if(!serial || level || slice) return;
    id<MTLTexture> target=tex_mtl(R.rt.color[0]);
    if(!target || target.textureType!=MTLTextureType2D || target.sampleCount!=1 ||
       fabs((double)target.width/target.height/((double)W/H)-1)>.01) return;
    if(S.has_vp && (S.vp[0][0]!=0 || S.vp[0][1]!=0 || S.vp[0][2]!=target.width || S.vp[0][3]!=target.height)) return;
    if(R.has_sc && (R.sc.x>0 || R.sc.y>0 || R.sc.w<(int)target.width || R.sc.h<(int)target.height)) return;
    static uint64_t last_frame=UINT64_MAX,last_serial;
    uint64_t frame=atomic_load(&g_cur_frame);if(frame==last_frame && serial==last_serial) return;
    if(!scene_pan_prepare(target,dx/W,dy/H)) return;
    rp_end();
    BENC b=blit();enc_b_copy_texture(b,target,0,0,MTLOriginMake(0,0,0),MTLSizeMake(target.width,target.height,1),g_scene_pan_copy,0,0,MTLOriginMake(0,0,0));
    enc_b_end(g_blit);g_blit=nil;ser_gate();
    RPD *rp=[RPD new];rp.colorAttachments[0].texture=target;
    rp.colorAttachments[0].loadAction=MTLLoadActionDontCare;rp.colorAttachments[0].storeAction=MTLStoreActionStore;
    RENC en=[g_cb renderCommandEncoderWithDescriptor:rp];if(!en) return;
#if DM_NVN_M3
    [en setFragmentTexture:g_scene_pan_copy atIndex:0];[en setFragmentSamplerState:g_scene_pan_sampler atIndex:0];
    [en setFragmentBuffer:g_scene_pan_uv offset:0 atIndex:0];
#else
    [en barrierAfterQueueStages:MTLStageAll beforeStages:MTLStageAll visibilityOptions:MTL4VisibilityOptionDevice];
    [g_scene_pan_args setTexture:g_scene_pan_copy.gpuResourceID atIndex:0];
    [g_scene_pan_args setSamplerState:g_scene_pan_sampler.gpuResourceID atIndex:0];
    [g_scene_pan_args setAddress:g_scene_pan_uv.gpuAddress atIndex:0];
    [en setArgumentTable:g_scene_pan_args atStages:MTLRenderStageFragment];
#endif
    [en setViewport:(MTLViewport){0,0,target.width,target.height,0,1}];
    [en setRenderPipelineState:g_scene_pan_pso];[en drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];m4_end(en);
    last_frame=frame;last_serial=serial;R.clear_c[0]=0;
    args_dirty_all();g_pso_dirty=g_dss_dirty=1;
    static int once;if(!once++) MLOG("Portrait camera pan: scene right %.0f/down %.0f pixels, HUD unchanged",(double)dx,(double)dy);
}
