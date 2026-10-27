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
 dm_orientation_entry_observe(&gate,0,1);assert(!gate.started && !gate.enabled);
 dm_orientation_entry_observe(&gate,1,0);assert(gate.started && gate.enabled); // Normal landscape entry already supports rotation.
 dm_orientation_entry_observe(&gate,1,1);assert(gate.enabled);
 dm_orientation_entry_observe(&gate,0,-1);assert(gate.enabled); // Map loading does not revoke permission.
 dm_orientation_entry_reset(&gate);
 dm_orientation_entry_observe(&gate,1,0);assert(gate.enabled); // Missed frontend tick cannot deadlock.
 dm_orientation_entry_reset(&gate);
 dm_orientation_entry_observe(&gate,1,1);assert(gate.started && !gate.enabled); // Portrait device on entry cannot snap.
 dm_orientation_entry_observe(&gate,1,1);assert(!gate.enabled);
 dm_orientation_entry_observe(&gate,1,0);assert(gate.enabled);
 dm_orientation_entry_observe(&gate,1,1);assert(gate.enabled);
 dm_orientation_entry_reset(&gate);
 dm_orientation_entry_observe(&gate,1,-1);assert(!gate.started && !gate.enabled);
 dm_orientation_entry_observe(&gate,1,0);assert(gate.enabled); // Delayed first sensor reading.
 dm_orientation_entry_reset(&gate);
 dm_orientation_entry_observe(&gate,1,-1);dm_orientation_entry_observe(&gate,1,1);assert(!gate.enabled);
 dm_orientation_entry_observe(&gate,1,0);assert(gate.enabled);
 dm_orientation_entry_reset(&gate);
 dm_orientation_entry_observe(&gate,1,0);assert(gate.enabled); // Late old-run frame is harmless.
 dm_orientation_entry_observe(&gate,0,0);assert(!gate.started && !gate.enabled);
 dm_orientation_entry_observe(&gate,1,1);assert(!gate.enabled); // Next portrait entry still stays landscape.
 puts("PASS orientation: immediate landscape permission, first rotation, missed frontend ticks, unknown readings, portrait entry hold, map loading and reset");
}
