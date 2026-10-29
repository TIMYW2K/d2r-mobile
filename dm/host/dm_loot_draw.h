#ifndef DM_LOOT_DRAW_H
#define DM_LOOT_DRAW_H
#include <stdint.h>
// Forward the observed draw arguments unchanged. A future replacement must
// explicitly prove that a corresponding frame is visible before suppressing.
typedef void (*dm_loot_native_draw_fn)(uint64_t,uint64_t,uint64_t);
static inline void dm_loot_native_forward(dm_loot_native_draw_fn fn,const uint64_t *regs) {
    fn(regs[0],regs[1],regs[2]);
}
#endif
