// Exercise the actual ui_safe_pump and move guard, rather than copying their arithmetic.
#include "../dm_cursor.c"
module_t g_mod[MAX_MOD]; int g_nmod;
static int portrait=1, controller, busy, ingame=1;
static uint64_t frame=100;
uint64_t dm_guest_frame(void) { return frame; }
uint64_t now_ns(void) { return frame*16666667; }
int dm_padmode_busy(void) { return busy; }
int dm_padmode_current(void) { return controller; }
int dm_uiscreen_ingame(void) { return ingame; }
int dm_uiscreen_portrait(void) { return portrait; }
int dm_uiscreen_panel(void) { return 0; }
void dm_hud_get(dm_hud_info *out) { memset(out,0,sizeof *out); }
int dm_screen_real(int *w,int *h) { *w=1206; *h=2622; return 1; }
_Atomic uint64_t g_ui_hover_xy;
int dm_cursor_item(dm_cursor_item_info *out) { (void)out; return 0; }
int dm_ui_hover_now(int x,int y) { (void)x; (void)y; return 0; }

void dm_ui_rect(int *x,int *y,int *w,int *h) { *x=0; *y=1288; *w=1206; *h=1334; }
static int checks,fails,relayouts;
#define CHECK(c) do { checks++; if(!(c)) { fails++; printf("FAIL line %d: %s\n",__LINE__,#c); } } while(0)
static _Alignas(8) unsigned char root[160], hud[160], nodes[7][160];
static uint64_t root_children[4], hud_children[7], vtable[4], panel_vtable[4];
static int panel_hooks;   // panels' vtable +0x10 must never be called: on real panels it is a destructive post-load hook
static void panel_relayout(uint64_t w) { (void)w; panel_hooks++; }
static const char *names[7]={"HealthBall","ManaBall","Belt","TouchPortraitBeltBacking0","TouchPortraitBeltBacking1","TouchPortraitBeltBacking2","TouchPortraitBeltBacking3"};
static const int base_x[7]={-900,600,-500,29543,29740,29936,30133};
static uint64_t ptr(void *p) { return (uint64_t)(uintptr_t)p; }
static int32_t *x(void *p) { return (int32_t *)((unsigned char *)p+0x70); }
static void relayout(uint64_t w) { (void)w; relayouts++; }
static void widget(void *v,const char *name,uint64_t parent) {
 unsigned char *p=v; memset(p,0,160); *(uint64_t *)p=ptr(vtable); *(const char **)(p+8)=name;
 *(uint64_t *)(p+0x30)=parent; *(float *)(p+0x80)=1.0f; p[0x50]=1;
}
static void tick(void) { frame+=6; ui_safe_pump(&g_mod[0],ptr(root),frame); }
static void shifted(void) {
 for(int i=0;i<7;i++) CHECK(*x(nodes[i])==base_x[i]+(i<2?55:i==2?8:73)-(i>=3?30000:0));
 CHECK(mv_count(MV_SAFE)==7);
}
static void restored(void) {
 for(int i=0;i<7;i++) CHECK(*x(nodes[i])==base_x[i]);
 CHECK(mv_count(MV_SAFE)==0);
}
int main(void) {
 unsigned char *base=calloc(1,0x2f05460); if(!base)return 2;
 g_nmod=1; strcpy(g_mod[0].name,"main"); g_mod[0].base=base;
 vtable[2]=ptr(relayout); panel_vtable[2]=ptr(panel_relayout);
 widget(root,"Root",0); widget(hud,"HUDPanel",ptr(root));
 *(uint64_t *)(base+0x2f05458)=ptr(root);
 root_children[0]=ptr(hud); *(uint64_t *)(root+0x58)=ptr(root_children); *(uint64_t *)(root+0x60)=1;
 *(uint64_t *)(hud+0x58)=ptr(hud_children); *(uint64_t *)(hud+0x60)=7;
 for(int i=0;i<7;i++) { widget(nodes[i],names[i],ptr(hud)); *x(nodes[i])=base_x[i]; hud_children[i]=ptr(nodes[i]); }
 *(float *)(nodes[0]+0x80)=1.4f; *(float *)(nodes[1]+0x80)=1.4f; *(float *)(nodes[2]+0x80)=1.6f;
 dm_ui_set_band_height_pt(444.6667f); CHECK(ui_pt_units(5)==24);
 // Compare the new translations against independently measured build 22.1
 // screenshot centers (1206x2622) and the v4 art's slot/orb centers.
 double unit=1334.0/2160.0, art=1334.0/1320.0;
 double orb_old=lround(ui_pt_units(15)/0.7)*unit;
 double orb_new=(ui_pt_units(15)+dm_hud_px_to_units(DM_PORTRAIT_ORB_CONTENT_DX_PX,1334))*unit;
 double belt_old=lround(ui_pt_units(15)/1.6)*unit;
 double belt_new=(ui_pt_units(15)+dm_hud_px_to_units(DM_PORTRAIT_BELT_CONTENT_DX_PX,1334))*unit;
 CHECK(fabs((186.9 + orb_new-orb_old) - (603+(949-1434)*art+45)) < 3);
 CHECK(fabs((1080.1 + orb_new-orb_old) - (603+(1831-1434)*art+45)) < 3);
 CHECK(fabs((444.9 + belt_new-belt_old) - (603+(1210-1434)*art+45)) < 3);
 CHECK(fabs((565.9 + belt_new-belt_old) - (603+(1330-1434)*art+45)) < 3);
 tick(); shifted(); CHECK(relayouts==7);
 // Repeated pumping must not accumulate another portrait shift.
 for(int i=0;i<10;i++) { tick(); shifted(); } CHECK(relayouts==7);
 portrait=0; tick(); restored(); CHECK(relayouts==14);
 portrait=1; tick(); shifted(); CHECK(relayouts==21);
 controller=1; tick(); restored(); CHECK(relayouts==28);
 controller=0; tick(); shifted(); CHECK(relayouts==35);
 busy=1; tick(); restored(); CHECK(relayouts==42);
 busy=0; tick(); shifted();
 ingame=0; tick(); restored();
 // Another orientation/return cycle exercises repopulating the guard after restoration.
 ingame=1; portrait=0; tick(); restored(); portrait=1; tick(); shifted();
 static _Alignas(8) unsigned char warnings[160],hire[160],merc[160]; static uint64_t pets[1];
 widget(warnings,"HUDWarnings",ptr(root)); *x(warnings)=100; *(uint64_t *)warnings=ptr(panel_vtable);
 widget(hire,"HireablesPanel",ptr(root)); *(uint64_t *)hire=ptr(panel_vtable); widget(merc,"Pet",ptr(hire)); *x(merc)=40; *(int32_t *)(merc+0x74)=60;
 pets[0]=ptr(merc); *(uint64_t *)(hire+0x58)=ptr(pets); *(uint64_t *)(hire+0x60)=1;
 root_children[1]=ptr(warnings); root_children[2]=ptr(hire); *(uint64_t *)(root+0x60)=3;
 tick(); CHECK(*x(warnings)==-107); CHECK(*x(merc)==172); CHECK(*(int32_t *)(merc+0x74)==-197);
 tick(); CHECK(*x(warnings)==-107); CHECK(*x(merc)==172); CHECK(*(int32_t *)(merc+0x74)==-197);
 portrait=0; tick(); CHECK(*x(warnings)==100); CHECK(*x(merc)==40); CHECK(*(int32_t *)(merc+0x74)==60);
 // 23.1 passed relayout=1 for the panels; HireablesPanel's hook rebuilt its portrait list from an already-destroyed
 // Template and crashed on the first portrait frame. Only the merc leaf widgets may be re-laid out.
 int n0=relayouts; portrait=1; tick();
 CHECK(relayouts==n0+9); CHECK(panel_hooks==0);   // 7 HUD leaves + 2 merc fields; the panel itself contributes none
 // Same for the top-lift path: moving the panels must not invoke either hook.
 *(int32_t *)(warnings+0x84)=-100; *(int32_t *)(hire+0x84)=-100; *(int32_t *)(warnings+0x74)=0; *(int32_t *)(hire+0x74)=0;
 ui_lift_pump(ptr(root),frame+=6);
 CHECK(*(int32_t *)(warnings+0x74)<0 && *(int32_t *)(hire+0x74)<0);
 CHECK(panel_hooks==0);
 mv_restore(ptr(root),MV_LIFT); CHECK(panel_hooks==0);
 // Exercise the production map publisher using the portrait band geometry
 // captured from the device, rather than a hand-written expected draw rect.
 static _Alignas(8) unsigned char map[0x1600];
 widget(map,"AutoMap",ptr(root)); *(uint64_t *)map=ptr(base)+0x20c58a0;
 *x(map)=-650; *(int32_t *)(map+0x74)=78;
 *(int32_t *)(map+0x78)=650; *(int32_t *)(map+0x7c)=718;
 *(int32_t *)(base+0x2d479f4)=2; *(int32_t *)(base+0x2d5c748)=1;
 root_children[3]=ptr(map); *(uint64_t *)(root+0x60)=4;
 portrait=1; tick();
 int mr[4],delta=0; CHECK(dm_ui_minimap_render_rect(mr,&delta));
 CHECK(mr[0]==402 && mr[1]==48 && mr[2]==401 && mr[3]==443);
 CHECK(1288+mr[1]+delta==210); // top 8% of 2622; safely in the red box
 CHECK(1288+mr[1]+delta+mr[3]<=654);
 // Hit region follows the unshifted map until a matching GPU draw confirms relocation.
 float hit[4]; ui_minimap_pump(&g_mod[0],ptr(root),1206,1334,frame);
 CHECK(dm_ui_minimap_rect(hit)); CHECK(fabsf(hit[1]-48.0f/1334)<0.001f);
 dm_ui_minimap_render_applied(); frame+=6; ui_minimap_pump(&g_mod[0],ptr(root),1206,1334,frame);
 CHECK(dm_ui_minimap_rect(hit)); CHECK(fabsf(hit[1]-(48.0f+delta)/1334)<0.001f);
 frame+=18; ui_minimap_pump(&g_mod[0],ptr(root),1206,1334,frame);
 CHECK(dm_ui_minimap_rect(hit)); CHECK(fabsf(hit[1]-48.0f/1334)<0.001f);
 for(int i=0;i<10;i++) { tick(); int next[4],nd; CHECK(dm_ui_minimap_render_rect(next,&nd)); CHECK(!memcmp(mr,next,sizeof mr)&&delta==nd); }
 // Both native mini modes share the same portrait position, fullscreen does not.
 *(int32_t *)(base+0x2d479f4)=1; tick(); CHECK(dm_ui_minimap_render_rect(mr,&delta)); CHECK(mr[0]==402);
 *(int32_t *)(base+0x2d5c748)=0; tick(); CHECK(!dm_ui_minimap_render_rect(mr,&delta));
 *(int32_t *)(base+0x2d479f4)=2; *(int32_t *)(base+0x2d5c748)=1;
 tick(); CHECK(dm_ui_minimap_render_rect(mr,&delta));
 portrait=0; tick(); CHECK(!dm_ui_minimap_render_rect(mr,&delta));
 free(base); printf("t_portrait_hud: %d checks, %d failures\n",checks,fails); return fails!=0;
}
