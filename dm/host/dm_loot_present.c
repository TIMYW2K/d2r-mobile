#include "dm_loot_present.h"
#include <string.h>
#include <math.h>
static int validtime(const dm_loot_present *p,uint64_t epoch,uint64_t now) {
    return epoch && p->epoch==epoch && p->committed_ns && now>=p->committed_ns &&
           now-p->committed_ns<=DM_LOOT_PRESENT_MAX_AGE_NS;
}
int dm_loot_present_init(dm_loot_present *p) {
    if(!p)return 0;
    memset(p,0,sizeof *p);return pthread_mutex_init(&p->lock,NULL)==0;
}
void dm_loot_present_destroy(dm_loot_present *p) { if(p)pthread_mutex_destroy(&p->lock); }
void dm_loot_present_invalidate(dm_loot_present *p) {
    if(!p || pthread_mutex_lock(&p->lock))return;
    p->epoch=0;p->committed_ns=0;p->visible.geometry.count=0;
    pthread_mutex_unlock(&p->lock);
}
int dm_loot_present_commit(dm_loot_present *p,const dm_loot_owned_snapshot *frame,
                           const unsigned char *eligible,uint64_t epoch,uint64_t ns) {
    uint16_t index[DM_LOOT_HASH_CAP];
    if(!p || !frame || !eligible || !epoch || !ns ||
       !dm_loot_index(&frame->geometry,index))return 0;
    // Validate all candidate strings before publishing any part of the frame.
    for(unsigned i=0;i<frame->geometry.count;i++) if(eligible[i]) {
        const unsigned char *s=(const unsigned char *)frame->labels[i].text;
        const unsigned char *end=memchr(s,0,DM_LOOT_INLINE_TEXT);
        if(!end || end==s || !dm_loot_utf8_valid(s,(size_t)(end-s)))return 0;
        for(unsigned k=0;k<4;k++)if(frame->labels[i].color[k]>255)return 0;
    }
    if(pthread_mutex_lock(&p->lock))return 0;
    p->visible.geometry.count=0;p->visible.geometry.viewport=frame->geometry.viewport;
    p->visible.geometry.time_ns=ns;
    for(unsigned i=0;i<frame->geometry.count;i++)if(eligible[i]) {
        unsigned j=p->visible.geometry.count++;
        p->visible.geometry.labels[j]=frame->geometry.labels[i];
        p->visible.labels[j]=frame->labels[i];
    }
    p->epoch=epoch;p->committed_ns=ns;
    pthread_mutex_unlock(&p->lock);return 1;
}
int dm_loot_present_can_hide(dm_loot_present *p,const void *record,size_t bytes,
                             uint64_t epoch,uint64_t now,float ui_dy) {
    if(!p || !record || bytes<DM_LOOT_RECORD_SIZE || !isfinite(ui_dy) ||
       pthread_mutex_trylock(&p->lock))return 0;
    int yes=0;
    if(validtime(p,epoch,now)) {
        const unsigned char *r=record;
        uint32_t id;memcpy(&id,r+0x10,4);
        const unsigned char *end=memchr(r+0x24,0,DM_LOOT_INLINE_TEXT);
        if(end) for(unsigned i=0;i<p->visible.geometry.count;i++)
            if(p->visible.geometry.labels[i].id==id) {
                const dm_loot_owned_label *l=&p->visible.labels[i];
                size_t n=(size_t)(end-(r+0x24));
                dm_loot_rect shown=p->visible.geometry.labels[i].rect;
                float dx=(float)dm_loot_i32(r)-shown.x;
                float dy=(float)dm_loot_i32(r+4)+ui_dy-shown.y;
                yes=dx*dx+dy*dy<=96.0f*96.0f &&
                    strlen(l->text)==n && !memcmp(l->text,r+0x24,n) &&
                    !memcmp(l->style,r+0x14,DM_LOOT_STYLE_BYTES) &&
                    !memcmp(l->color,r+0xac,sizeof l->color);
                break;
            }
    }
    pthread_mutex_unlock(&p->lock);return yes;
}
int dm_loot_present_hit(dm_loot_present *p,float x,float y,float min_size,
                        uint64_t epoch,uint64_t now,uint32_t *id) {
    if(!p || !id || !isfinite(x) || !isfinite(y) || !isfinite(min_size) ||
       min_size<0 || pthread_mutex_trylock(&p->lock))return 0;
    int found=0;float best=INFINITY;
    if(validtime(p,epoch,now)) for(unsigned i=0;i<p->visible.geometry.count;i++) {
        dm_loot_rect r=p->visible.geometry.labels[i].rect;
        float cx=r.x+r.w*0.5f,cy=r.y+r.h*0.5f;
        float hw=fmaxf(r.w,min_size)*0.5f,hh=fmaxf(r.h,min_size)*0.5f;
        float dx=x-cx,dy=y-cy,d=dx*dx+dy*dy;
        if(fabsf(dx)<=hw && fabsf(dy)<=hh && d<best) {
            best=d;*id=p->visible.geometry.labels[i].id;found=1;
        }
    }
    pthread_mutex_unlock(&p->lock);return found;
}
