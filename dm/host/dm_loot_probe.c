#include "dm_loot_probe.h"
#include "dm_loot_exchange.h"
#include <pthread.h>
#include <stdatomic.h>

static dm_loot_exchange g_exchange;
static pthread_once_t g_once=PTHREAD_ONCE_INIT;
static int g_ready;
static dm_loot_owned_snapshot g_stage; // producer only
static dm_loot_pair g_pair;            // UI consumer only
static dm_loot_owned_snapshot g_plan;  // UI consumer only
static _Atomic uint64_t g_frames,g_invalid,g_ticks,g_samples,g_last_count;
static void initialize(void) { g_ready=dm_loot_exchange_init(&g_exchange); }

void dm_loot_probe_frame(const void *records,size_t bytes,unsigned count,
                         uint32_t viewport,float ui_dy,uint64_t ns) {
    pthread_once(&g_once,initialize);
    if (!g_ready) return;
    atomic_fetch_add_explicit(&g_frames,1,memory_order_relaxed);
    if (!dm_loot_capture_owned(records,bytes,count,ns,viewport,ui_dy,&g_stage) ||
        !dm_loot_exchange_publish(&g_exchange,&g_stage)) {
        atomic_fetch_add_explicit(&g_invalid,1,memory_order_relaxed);
        dm_loot_exchange_invalidate(&g_exchange); return;
    }
    atomic_store_explicit(&g_last_count,g_stage.geometry.count,memory_order_relaxed);
}
void dm_loot_probe_reset(void) {
    pthread_once(&g_once,initialize);
    if (g_ready) dm_loot_exchange_invalidate(&g_exchange);
}
void dm_loot_probe_tick(uint64_t ns) {
    pthread_once(&g_once,initialize);
    if (!g_ready) return;
    atomic_fetch_add_explicit(&g_ticks,1,memory_order_relaxed);
    if (dm_loot_exchange_copy(&g_exchange,&g_pair,ns) &&
        dm_loot_pair_sample(&g_pair,ns,&g_plan) &&
        dm_loot_exchange_pair_valid(&g_exchange,&g_pair))
        atomic_fetch_add_explicit(&g_samples,1,memory_order_relaxed);
}
void dm_loot_probe_stats(uint64_t *frames,uint64_t *invalid,uint64_t *ticks,
                         uint64_t *samples,uint64_t *last_count) {
    if(frames)*frames=atomic_load_explicit(&g_frames,memory_order_relaxed);
    if(invalid)*invalid=atomic_load_explicit(&g_invalid,memory_order_relaxed);
    if(ticks)*ticks=atomic_load_explicit(&g_ticks,memory_order_relaxed);
    if(samples)*samples=atomic_load_explicit(&g_samples,memory_order_relaxed);
    if(last_count)*last_count=atomic_load_explicit(&g_last_count,memory_order_relaxed);
}
