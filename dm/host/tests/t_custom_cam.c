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
int main(int argc,char **argv) {
    assert(argc==2); char path[1200]; snprintf(path,sizeof path,"%s/sw_custom.txt",argv[1]);
    FILE *f=fopen(path,"w"); assert(f); fputs("cam=2\nq3d=2\nmetalfx=1\n",f); fclose(f);
    dm_custom_init(argv[1]); assert(dm_custom_get("cam")==2); assert(current_cam==2); assert(current_quality==2);
    // Windows rename cannot replace an existing destination; iOS POSIX rename can.
    assert(remove(path)==0);
    atomic_store(&g_pend[K_CAM],1); dm_custom_on_uimsg("OptionsPanelMessage","AcceptChanges","");
    assert(dm_custom_get("cam")==1 && current_cam==1 && dm_custom_get("q3d")==2);
    f=fopen(path,"r"); assert(f); char line[128]; int found=0; while(fgets(line,sizeof line,f)) if(!strcmp(line,"cam=1\n")) found=1; fclose(f); assert(found); assert(remove(path)==0);
    dm_custom_on_uimsg("OptionsPanelMessage","ResetToDefaults",""); dm_custom_on_uimsg("OptionsPanelMessage","AcceptChanges","");
    assert(dm_custom_get("cam")==0 && current_cam==0 && dm_custom_get("q3d")==0);
    remove(path); puts("PASS camera and quality settings load, apply, persist and reset"); return 0;
}
