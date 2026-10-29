#include "dm_loot_draw.h"
#include "dm_loot_live.h"
#include "hle.h"
#include <string.h>

// The original draw is forwarded unless the independent layer acknowledged
// the same label, style, location and current scene within its short TTL.
void svc_loot_draw(uint64_t *regs) {
    static uint8_t *base;
    if (!base) for (int i=0;i<g_nmod;i++)
        if (!strcmp(g_mod[i].name,"main")) { base=(uint8_t *)g_mod[i].base; break; }
    if (!base) die("loot label draw: missing main module");
    uint64_t now_ns(void);
    if (dm_loot_live_hide((const void *)(uintptr_t)regs[0],now_ns())) return;
    dm_loot_native_forward((dm_loot_native_draw_fn)(base+0x43b850),regs);
}
