// Exercise actual camera mode transitions and published projection matrices.
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
int dm_screen_real(int *w, int *h) { *w=1206; *h=2622; return 1; }
int dm_ui_map_geom(float *cell) { (void)cell; return 0; }
int dm_ui_minimap_rect(float *rect) { (void)rect; return 0; }
int dm_cam_mode(void);
void dm_cam_set_mode(int);
void dm_cam_publish(const float *, const float *, const float *, const float *, float, float);
int dm_cam_label_matrix(const float *, float *);
void dm_cam_remap_stick(int32_t *, int32_t *, int32_t *, int32_t *);
int main(void) {
    float ident[16]={1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1}, perspective[16]={1,0,0,0, 0,2,0,0, 0,0,3,0, 4,5,6,1}, out[16];
    assert(dm_cam_mode()==0); assert(!dm_cam_label_matrix(ident,out));
    int32_t lx=12000,ly=-7000,rx=9000,ry=-4000;
    dm_cam_remap_stick(&lx,&ly,&rx,&ry); assert(lx==12000 && ly==-7000 && rx==9000 && ry==-4000);
    dm_cam_set_mode(1); assert(dm_cam_mode()==1); assert(!dm_cam_label_matrix(ident,out));
    dm_cam_publish(ident,ident,perspective,ident,0,0); assert(dm_cam_label_matrix(ident,out));
    for(int i=0;i<16;i++) assert(fabsf(out[i]-perspective[i])<0.00001f);
    dm_cam_set_mode(2); assert(dm_cam_mode()==2); assert(dm_cam_label_matrix(ident,out));
    dm_cam_set_mode(0); assert(dm_cam_mode()==0 && !dm_cam_label_matrix(ident,out));
    dm_cam_set_mode(3); assert(dm_cam_mode()==0);
    puts("PASS camera mode transitions and projection publication"); return 0;
}
