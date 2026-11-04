// Verify camera mode persistence and Apply/Default alongside the local q3d mode.
#include "../dm_custom.c"
#include <assert.h>
#include <stdio.h>
static int current_cam=-1, current_quality=-1;
void dm_cam_set_mode(int v) { current_cam=v; }
void dm_3d_quality_mode_set(int v) { current_quality=v; }
int dm_3d_quality_mode_get(void) { return current_quality; }
float dm_3d_scale_cur(void) { return 1.0f; }
int dm_lang_zh(void) { return 1; }
const char *dm_save_commitdir(void) { return 0; }
static void persisted(const char *path,const char *expected) {
 FILE *f=fopen(path,"r");assert(f);char buf[128];int found=0;
 while(fgets(buf,sizeof buf,f)) if(!strcmp(buf,expected))found=1;
 fclose(f);assert(found);
}
int main(int argc,char **argv) {
 assert(argc==2);char path[1200];snprintf(path,sizeof path,"%s/sw_custom.txt",argv[1]);
 FILE *f=fopen(path,"w");assert(f);fputs("cam=2\nq3d=2\n",f);fclose(f);
 dm_custom_init(argv[1]);assert(dm_custom_get("allow_portrait")==1);assert(dm_custom_get("bright")==2);
 assert(dm_custom_get("cam")==2 && dm_custom_get("show_fps")==0);
 atomic_store(&g_pend[K_ALLOW_PORTRAIT],0);assert(dm_custom_get("allow_portrait")==1);
 dm_custom_on_uimsg("OptionsPanelMessage","CancelChanges","");assert(atomic_load(&g_pend[K_ALLOW_PORTRAIT])==-1);
 assert(remove(path)==0);atomic_store(&g_pend[K_ALLOW_PORTRAIT],0);atomic_store(&g_pend[K_BRIGHT],1);
 dm_custom_on_uimsg("OptionsPanelMessage","AcceptChanges","");
 assert(dm_custom_get("allow_portrait")==0 && dm_custom_get("bright")==1);
 persisted(path,"allow_portrait=0\n");persisted(path,"bright=1\n");
 atomic_store(&g_loaded,0);assert(dm_custom_get("allow_portrait")==0 && dm_custom_get("bright")==1);
 assert(dm_custom_get("cam")==2 && dm_custom_get("q3d")==2);
 dm_custom_on_uimsg("OptionsPanelMessage","ResetToDefaults","");assert(dm_custom_get("allow_portrait")==0);
 assert(remove(path)==0);dm_custom_on_uimsg("OptionsPanelMessage","AcceptChanges","");
 assert(dm_custom_get("allow_portrait")==1 && dm_custom_get("bright")==2);
 assert(dm_custom_get("cam")==0 && dm_custom_get("q3d")==0);
 persisted(path,"allow_portrait=1\n");assert(remove(path)==0);
 f=fopen(path,"w");assert(f);fputs("bright=99\nallow_portrait=0\n",f);fclose(f);atomic_store(&g_loaded,0);
 assert(dm_custom_get("bright")==2 && dm_custom_get("allow_portrait")==0);remove(path);
 puts("PASS portrait/brightness: defaults, staging, discard, apply, disk reload, reset, invalid data and independent camera/FPS");
 return 0;
}
