#include "../dm_custom.c"
#include <assert.h>
module_t g_mod[MAX_MOD]; int g_nmod;
static unsigned char base[1];
static _Alignas(8) unsigned char root[160],panel[160],life[3072],mana[3072],fps[3072];
static uint64_t roots[1],nodes[3];
static void setup(unsigned char *w,const char *name) { *(const char **)(w+8)=name;w[0x50]=1; }
int main(void) {
 strcpy(g_mod[0].name,"main");g_mod[0].base=base;g_nmod=1;
 setup(root,"Root");setup(panel,"GameplayOptionsPanel");setup(life,"ShowLifeOrbText");setup(mana,"ShowManaOrbText");setup(fps,"ShowManaOrbText");
 roots[0]=(uint64_t)panel;*(uint64_t *)(root+0x58)=(uint64_t)roots;*(uint64_t *)(root+0x60)=1;
 nodes[0]=(uint64_t)fps;nodes[1]=(uint64_t)life;nodes[2]=(uint64_t)mana;*(uint64_t *)(panel+0x58)=(uint64_t)nodes;*(uint64_t *)(panel+0x60)=3;
 *(uint64_t *)life=*(uint64_t *)mana=*(uint64_t *)fps=(uint64_t)base+DM_VT_TOGGLE;
 *(int32_t *)(life+0x70)=*(int32_t *)(mana+0x70)=-99999;
 dm_custom_prepare_ui_message((uint64_t)root,"OptionsPanelMessage","SetShowLifeOrbText");assert(life[0xa88] && !mana[0xa88] && !fps[0xa88]);
 dm_custom_prepare_ui_message((uint64_t)root,"OptionsPanelMessage","SetShowManaOrbText");assert(mana[0xa88] && !fps[0xa88]);
 assert(*(int32_t *)(life+0x610)==4);
 life[0xa88]=0;*(uint64_t *)life=0;dm_custom_prepare_ui_message((uint64_t)root,"OptionsPanelMessage","SetShowLifeOrbText");assert(!life[0xa88]);
 mana[0xa88]=0;dm_custom_prepare_ui_message((uint64_t)root,"Other","SetShowManaOrbText");assert(!mana[0xa88]);
 panel[0x50]=0;dm_custom_prepare_ui_message((uint64_t)root,"OptionsPanelMessage","SetShowManaOrbText");assert(!mana[0xa88]);
 puts("PASS orb anchor delivery: both native options, exact hidden owner, FPS duplicate unchanged, type/category/visibility guards");
}
