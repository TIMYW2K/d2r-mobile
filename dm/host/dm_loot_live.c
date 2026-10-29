#include "dm_loot_live.h"
#include "dm_loot_present.h"
#include "dm_loot_probe.h"
#include "dm_uiscreen.h"
#include <pthread.h>
#include <stdatomic.h>
static dm_loot_present present;
static pthread_once_t once=PTHREAD_ONCE_INIT;
static int ready;
static _Atomic uint64_t active_epoch;
static _Atomic uint64_t n_commits,n_eligible,n_seen,n_hidden;
int dm_focus_now(void);
static void initialize(void){ready=dm_loot_present_init(&present);}
void dm_loot_live_invalidate(void){
    atomic_store_explicit(&active_epoch,0,memory_order_release);
    pthread_once(&once,initialize);
    if(ready)dm_loot_present_invalidate(&present);
}
int dm_loot_live_commit(const dm_loot_owned_snapshot *f,const unsigned char *eligible,
                        uint64_t epoch,uint64_t ns){
    pthread_once(&once,initialize);
    if(!ready || !dm_uiscreen_ingame() || dm_uiscreen_panel() || !dm_focus_now() ||
       !dm_loot_probe_epoch_valid(epoch)) { dm_loot_live_invalidate(); return 0; }
    if(!dm_loot_present_commit(&present,f,eligible,epoch,ns) ||
       !dm_loot_probe_epoch_valid(epoch)) { dm_loot_live_invalidate(); return 0; }
    uint64_t n=0; for(unsigned i=0;i<f->geometry.count;i++) n+=eligible[i]!=0;
    atomic_store_explicit(&n_eligible,n,memory_order_relaxed);
    atomic_fetch_add_explicit(&n_commits,1,memory_order_relaxed);
    atomic_store_explicit(&active_epoch,epoch,memory_order_release);
    return 1;
}
int dm_loot_live_hide(const void *record,uint64_t ns){
    atomic_fetch_add_explicit(&n_seen,1,memory_order_relaxed);
    uint64_t epoch=atomic_load_explicit(&active_epoch,memory_order_acquire);
    if(!epoch || !dm_uiscreen_ingame() || dm_uiscreen_panel() || !dm_focus_now() ||
       !dm_loot_probe_epoch_valid(epoch))return 0;
    int yes=ready && dm_loot_present_can_hide(&present,record,DM_LOOT_RECORD_SIZE,epoch,ns,(float)dm_ui_proj_dy());
    if(yes)atomic_fetch_add_explicit(&n_hidden,1,memory_order_relaxed);
    return yes;
}
int dm_loot_live_hit(float x,float y,float min,uint64_t ns,uint32_t *id){
    uint64_t epoch=atomic_load_explicit(&active_epoch,memory_order_acquire);
    if(!epoch || !dm_uiscreen_ingame() || dm_uiscreen_panel() || !dm_focus_now() ||
       !dm_loot_probe_epoch_valid(epoch))return 0;
    return ready && dm_loot_present_hit(&present,x,y,min,epoch,ns,id);
}

void dm_loot_live_stats(uint64_t *commits,uint64_t *eligible,uint64_t *seen,uint64_t *hidden){
    if(commits)*commits=atomic_load_explicit(&n_commits,memory_order_relaxed);
    if(eligible)*eligible=atomic_load_explicit(&n_eligible,memory_order_relaxed);
    if(seen)*seen=atomic_load_explicit(&n_seen,memory_order_relaxed);
    if(hidden)*hidden=atomic_load_explicit(&n_hidden,memory_order_relaxed);
}
