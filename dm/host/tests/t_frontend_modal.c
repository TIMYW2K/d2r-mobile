// Test production root-panel gating: selecting Settings or a movie must not
// let automatic character-list recovery inject X into the new screen.
#include <assert.h>
#include "../dm_cursor.c"
module_t g_mod[MAX_MOD]; int g_nmod;
static _Alignas(8) unsigned char root[160], panels[3][160];
static uint64_t children[3];
static uint64_t ptr(void *p) { return (uint64_t)(uintptr_t)p; }
static void panel(int i,const char *name,int visible) {
 memset(panels[i],0,sizeof panels[i]); *(const char **)(panels[i]+8)=name;
 panels[i][0x50]=(unsigned char)visible; children[i]=ptr(panels[i]);
}
int main(void) {
 *(uint64_t *)(root+0x58)=ptr(children); *(uint64_t *)(root+0x60)=3;
 panel(0,"CharacterSelectPanel",0); panel(1,"MainMenuButtonRibbon",1); panel(2,"SettingsPanel",0);
 assert(!fe_modal_open(ptr(root))); assert(!fe_modal_open(0));
 static const char *names[]={"SettingsPanel","VideoOptionsPanel","SoundOptionsPanel","GameplayOptionsPanel","CreditsModal","CinematicsModal","MoviePlayerPanel","MoviePlayer","ConfirmationModal","DeleteCharacterModal","LoadScreenPanel"};
 for(unsigned i=0;i<sizeof names/sizeof names[0];i++) {
  panel(2,names[i],1); assert(fe_modal_open(ptr(root)));
  panel(2,names[i],0); assert(!fe_modal_open(ptr(root)));
 }
 panel(2,"CharacterSelectPanel",1); assert(!fe_modal_open(ptr(root)));
 puts("t_frontend_modal: settings/movie/modal visibility and closed-page recovery passed"); return 0;
}
