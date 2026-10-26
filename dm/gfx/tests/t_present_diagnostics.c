#include "../nvn/nvn_present_diagnostics.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
 assert(!nvn_present_dropped(33.34,1000.0/30)); assert(nvn_present_dropped(66.67,1000.0/30));
 assert(!nvn_present_dropped(16.67,1000.0/60)); assert(nvn_present_dropped(33.34,1000.0/60));
 assert(nvn_present_dropped(-1,1000.0/30));assert(!nvn_present_dropped(0,1000.0/60));
 assert(nvn_present_expected_period(16.67,30)>33.3);assert(nvn_present_expected_period(33.34,60)==33.34);
 unsigned points[5][2];int land[4]={0,0,2622,1206},port[4]={0,0,1206,2622},sub[4]={20,30,100,80};
 const int *crops[]={land,port,sub};
 for(int j=0;j<3;j++) { const int *c=crops[j]; assert(nvn_probe_points(2622,2622,c,points));
  for(int i=0;i<5;i++) { assert(points[i][0]>=(unsigned)c[0] && points[i][0]<(unsigned)(c[0]+c[2]));assert(points[i][1]>=(unsigned)c[1] && points[i][1]<(unsigned)(c[1]+c[3])); }
 }
 assert(points[0][0]==70 && points[0][1]==70);
 int bad[4]={-1,0,1206,2622};assert(!nvn_probe_points(2622,2622,bad,points));bad[0]=2000;assert(!nvn_probe_points(2622,2622,bad,points));
 assert(!nvn_probe_points(0,2622,0,points));assert(nvn_probe_points(1,1,0,points));assert(!points[4][0]&&!points[4][1]);
 puts("PASS presented diagnostics: target-aware 30/60 FPS, discarded frames, visible landscape/portrait/offset crops and invalid bounds");
}
