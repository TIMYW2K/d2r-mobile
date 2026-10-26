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
 puts("PASS orientation retry: active-only, flat ignored, delayed geometry retries, cap, changed posture and settled reset");
}
