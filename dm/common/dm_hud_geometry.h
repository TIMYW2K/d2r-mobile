#pragma once
#include <math.h>

/* Shared contract: UIKit points for the bar, real screen pixels for portrait
   offsets, and game layout units normalized to a UI-band height of 2160. */
#define DM_PORTRAIT_HUD_DX_PT 15.0f
/* Pixel calibration from 22.1 portrait screenshot after removing its old
   scale-divided translation; backing art and hit targets remain at the bar offset. */
#define DM_PORTRAIT_ORB_CONTENT_DX_PX (-11.0)
#define DM_PORTRAIT_BELT_CONTENT_DX_PX (-40.0)
#define DM_PORTRAIT_MERC_SCALE 1.32f
#define DM_PORTRAIT_MERC_RIGHT_PX 88.0
#define DM_PORTRAIT_MERC_UP_PX 155.0
#define DM_PORTRAIT_MERC_GAP_REDUCTION_PX 22.0
#define DM_PORTRAIT_AREA_LEFT_PX 95.0
#define DM_PORTRAIT_AREA_UP_PX 200.0

static inline int dm_hud_px_to_units(double px, int band_height_px) {
    return band_height_px > 0 ? (int)lround(px * 2160.0 / band_height_px) : 0;
}
static inline int dm_hud_merc_y_units(int band_height_px, int extra_up_units) {
    return band_height_px > 0 ? (int)lround((-DM_PORTRAIT_MERC_UP_PX * 2160.0 / band_height_px - extra_up_units) / DM_PORTRAIT_MERC_SCALE) : 0;
}
