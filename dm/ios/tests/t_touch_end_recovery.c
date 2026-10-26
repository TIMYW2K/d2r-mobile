#include "../Sources/dm_touch_end_policy.h"
#include "../Sources/dm_touch_controls.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
 assert(!dm_touch_end_recover(0,0,1,0));assert(!dm_touch_end_recover(1,1,1,0));assert(!dm_touch_end_recover(1,0,0,0));
 assert(dm_touch_end_recover(1,0,1,0));assert(dm_touch_end_recover(1,0,0,1));
 assert(dm_touch_end_time(1.2,1,2)==1.2);assert(dm_touch_end_time(0,1,2)==2);assert(dm_touch_end_time(3,1,2)==2);
 assert(dm_touch_end_defer(1,0,0));assert(!dm_touch_end_defer(1,0,1));assert(!dm_touch_end_defer(1,1,0));
 dm_tc_state s;dm_tc_init(&s,874,402);
 dm_tc_target left=s.layout.target[DM_TC_BELT1],right=s.layout.target[DM_TC_BELT2];
 dm_tc_begin_at(&s,0,left.x,left.y,1);dm_tc_begin_at(&s,1,right.x,right.y,1.05);
 dm_tc_end_at(&s,0,dm_touch_end_time(1.15,1,2));assert(!s.finger[0].active && s.finger[1].active);
 unsigned uses=s.use_seq;dm_tc_cancel_finger(&s,1);assert(!s.finger[1].active && s.use_seq==uses);
 dm_tc_cancel_finger(&s,1);assert(s.use_seq==uses);
 puts("PASS touch end recovery: owned-only, pending preserved, original timestamps, independent fingers, cancellation and no duplicate actions");
}
