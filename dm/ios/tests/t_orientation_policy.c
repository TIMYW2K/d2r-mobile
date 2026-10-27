#include "../Sources/dm_orientation_policy.h"
#include <assert.h>
#include <stdio.h>
static void stable(dm_orientation_state *s,int pose,double t) {dm_orientation_sample(s,pose,t);dm_orientation_sample(s,pose,t+.3);}
int main(void) {
 assert(dm_orientation_gravity(0,-1,0)==1);assert(dm_orientation_gravity(1,0,0)==0);assert(dm_orientation_gravity(-1,0,0)==0);
 assert(dm_orientation_gravity(0,-.4,.916)==1);assert(dm_orientation_gravity(.4,0,.916)==0);
 assert(dm_orientation_gravity(0,0,-1)==-1);assert(dm_orientation_gravity(0,1,0)==-1);assert(dm_orientation_gravity(.7,-.7,0)==-1);assert(dm_orientation_gravity(NAN,-1,0)==-1);
 dm_orientation_state s;dm_orientation_start(&s);assert(s.session && dm_orientation_target(&s)==0);
 stable(&s,1,0);assert(s.baseline==1 && dm_orientation_target(&s)==0); // Real log: raw UIDevice stays Portrait. Entry still landscape.
 stable(&s,1,1);assert(dm_orientation_target(&s)==0);
 stable(&s,0,2);assert(dm_orientation_target(&s)==0);
 stable(&s,1,3);assert(dm_orientation_target(&s)==1); // Fresh gravity works even when UIDevice never changes.
 dm_orientation_sample(&s,0,4);dm_orientation_sample(&s,-1,4.1);assert(dm_orientation_target(&s)==1); // Noise/flat cannot flip.
 stable(&s,0,5);assert(dm_orientation_target(&s)==0); // Portrait -> landscape.
 // HUD/loading and suspension never reset the session. Only explicit front-end exit does.
 stable(&s,1,6);assert(s.session && dm_orientation_target(&s)==1);
 dm_orientation_stop(&s);assert(!s.session && dm_orientation_target(&s)==0);
 dm_orientation_start(&s);stable(&s,1,7);assert(dm_orientation_target(&s)==0); // Old portrait choice cannot leak into next run.
 dm_orientation_retry retry={-1,0,0};
 for(int i=0;i<6;i++) {assert(dm_orientation_retry_step(&retry,s.session,1,0,i*.5));assert(!dm_orientation_retry_step(&retry,1,1,0,i*.5+.1));}
 assert(!dm_orientation_retry_step(&retry,1,1,0,3));assert(dm_orientation_retry_step(&retry,1,1,0,4.5)); // Backoff, never permanent lock.
 assert(!dm_orientation_retry_step(&retry,1,1,1,5));assert(retry.attempts==0);
 assert(!dm_orientation_retry_step(&retry,0,1,0,6));
 puts("PASS rotation: stale UIDevice replay, gravity hysteresis, initial landscape, both directions, explicit exit, loading/resume persistence and independent request retries");
}
