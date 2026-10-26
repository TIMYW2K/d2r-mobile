#include "../dm_cursor.c"
#include <assert.h>
module_t g_mod[MAX_MOD]; int g_nmod;
static void *producer(void *arg) { char text[96];for(int i=0;i<8;i++) { snprintf(text,sizeof text,"producer%ld:%d",(long)arg,i);dm_ui_send_queue(text); }return NULL; }
int main(void) {
 char a[96]="OptionsPanelMessage:SetShowLifeOrbText",out[96];dm_ui_send_queue(a);memset(a,0,sizeof a);
 dm_ui_send_queue("OptionsPanelMessage:SetShowManaOrbText");
 assert(ui_sendq_pop(out)&&!strcmp(out,"OptionsPanelMessage:SetShowLifeOrbText"));
 assert(ui_sendq_pop(out)&&!strcmp(out,"OptionsPanelMessage:SetShowManaOrbText"));assert(!ui_sendq_pop(out));
 pthread_t th[4];for(long i=0;i<4;i++)assert(!pthread_create(&th[i],0,producer,(void*)i));for(int i=0;i<4;i++)assert(!pthread_join(th[i],0));
 int next[4]={0},n=0,p,k;while(ui_sendq_pop(out)) { assert(sscanf(out,"producer%d:%d",&p,&k)==2);assert(k==next[p]++);n++; }assert(n==32);
 for(int i=0;i<33;i++)dm_ui_send_queue("full:queue");assert(g_sendq_count==32);while(ui_sendq_pop(out)){};
 dm_ui_send_queue(0);dm_ui_send_queue("");memset(a,'x',sizeof a);a[95]=0;dm_ui_send_queue(a);assert(ui_sendq_pop(out));assert(!ui_sendq_pop(out));
 puts("PASS native UI FIFO: orb messages both retained, caller ownership, four producers, per-producer ordering, bounded overflow and empty requests");
}
