// Camera profile regression: links the actual dm_uiscreen.c with inert game/UI dependency stubs.
// cc -std=c11 t_cam_zoom.c ../dm_uiscreen.c -lm -o t_cam_zoom
#include "../dm_uiscreen.h"
#include "../dm.h"
#include <math.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>

static int checks, fails, reset_count;
void dm_3d_scale_reset_level(void) { ++reset_count; }
// No game/UI is loaded: these unrelated dm_uiscreen dependencies are inert in this test.
module_t g_mod[MAX_MOD]; int g_nmod;
uint64_t front_game_res(int docked) { (void)docked; return 0; }
void dm_oe_res_changed(void) {}
int gate_obs_present(int id) { (void)id; return 0; }
void gate_obs_set(int id, void (*fn)(const uint64_t *)) { (void)id; (void)fn; }
#define CHECK(c, what) do { ++checks; if (!(c)) { ++fails; fprintf(stderr, "FAIL: %s\n", what); } } while (0)
static void expect(float want, const char *what, void *slot) {
    CHECK(fabsf(dm_cam_zoom_factor() - want) < 0.00001f, what);
    CHECK(fabsf(dm_cam_zoom_ratio() - 1.0f / want) < 0.00001f, "world-hit ratio matches extent");
    union { uint32_t u; float f; } live = { .u = atomic_load((_Atomic uint32_t *)slot) };
    CHECK(fabsf(live.f - want) < 0.00001f, "KZOOM sees the same extent");
    CHECK(slot == dm_cam_zoom_factor_slot(), "KZOOM address remains stable");
}
int main(void) {
    void *slot = dm_cam_zoom_factor_slot();
    CHECK(dm_cam_zoom_near_extent() == 1.0f, "near view keeps original range");
    CHECK(dm_cam_zoom_far_extent() == 1.25f, "landscape far default");
    expect(1.25f, "initial landscape", slot);
    dm_cam_zoom_set(1.0f);
    expect(1.0f, "landscape near", slot);
    dm_uiscreen_set_portrait(1);
    CHECK(dm_cam_zoom_far_extent() == 1.5f, "portrait far is 150 percent");
    expect(1.5f, "first portrait uses its own far default", slot);
    dm_uiscreen_set_portrait(0);
    expect(1.0f, "landscape near survives portrait", slot);
    dm_cam_zoom_reset();
    expect(1.25f, "landscape reset affects landscape only", slot);
    dm_uiscreen_set_portrait(1);
    dm_cam_zoom_set(1.0f);
    dm_uiscreen_set_portrait(0);
    expect(1.25f, "landscape far survives portrait near", slot);
    dm_uiscreen_set_portrait(1);
    expect(1.0f, "portrait near survives landscape", slot);
    dm_cam_zoom_set(INFINITY);
    expect(1.5f, "portrait extent clamps to 150 percent", slot);
    dm_cam_zoom_set(NAN);
    expect(1.0f, "non-finite invalid extent is safe", slot);
    dm_cam_zoom_reset();
    expect(1.5f, "portrait reset restores its own far", slot);
    dm_uiscreen_set_portrait(0);
    dm_cam_zoom_set(2.0f);
    expect(1.25f, "landscape remains capped at 125 percent", slot);
    int before = reset_count;
    dm_uiscreen_set_portrait(0);
    dm_cam_zoom_set(1.25f);
    CHECK(reset_count == before, "unchanged state avoids resetting dynamic scale");
    printf("camera profiles: %d checks, %d failures\n", checks, fails);
    return fails != 0;
}
