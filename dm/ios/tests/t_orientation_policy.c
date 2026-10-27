#include "../Sources/dm_orientation_policy.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
 dm_orientation_retry s={-1,0,0};
 assert(!dm_orientation_retry_step(&s,0,0,0,0));
 assert(!dm_orientation_retry_step(&s,1,-1,0,0));
 for(int i=0;i<6;i++) { assert(dm_orientation_retry_step(&s,1,0,0,i*.5)); assert(!dm_orientation_retry_step(&s,1,0,0,i*.5+.1)); }
 assert(!dm_orientation_retry_step(&s,1,0,0,10));
 assert(dm_orientation_retry_step(&s,1,1,0,11));
 assert(!dm_orientation_retry_step(&s,1,1,1,12)); assert(s.attempts==0);
 assert(dm_orientation_retry_step(&s,1,0,0,13));
 dm_orientation_entry gate={0,-1,0};
 dm_orientation_entry_observe(&gate,0,1); assert(!gate.started && !gate.enabled);
 dm_orientation_entry_observe(&gate,1,1); assert(gate.started && !gate.enabled);
 dm_orientation_entry_observe(&gate,1,1); assert(!gate.enabled); // Entry cannot snap portrait.
 dm_orientation_entry_observe(&gate,0,-1); assert(!gate.enabled); // Loading/flat preserves baseline.
 dm_orientation_entry_observe(&gate,1,0); assert(gate.enabled); // User rotates after entry.
 dm_orientation_entry_observe(&gate,0,1); assert(gate.enabled); // Map loading retains session.
 dm_orientation_entry_reset(&gate); assert(!gate.started && !gate.enabled);
 dm_orientation_entry_observe(&gate,1,1); assert(!gate.started); // Late old-run tick after exit.
 dm_orientation_entry_observe(&gate,0,1); assert(!gate.wait_frontend);
 dm_orientation_entry_observe(&gate,1,-1); assert(!gate.enabled);
 dm_orientation_entry_observe(&gate,1,1); assert(!gate.enabled); // First valid reading is baseline.
 dm_orientation_entry_observe(&gate,1,0); assert(gate.enabled); // Also works after resume.
 puts("PASS orientation retry: active-only, flat ignored, delayed geometry retries, cap, changed posture, settled reset, landscape entry and explicit-exit gate");
}
