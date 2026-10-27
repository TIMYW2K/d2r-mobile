#pragma once
#include <math.h>

/* Match the original, CPU-clipped minimap scissor in render-pass pixels. The viewport,
   not the allocated texture size, defines scale: a phone swapchain can be square. */
static inline int nvn_minimap_scissor_shift(const int rect[4], int rw, int rh,
                                           const double vp[4], const int sc[4], double ui_hfrac,
                                           int dy, long *shift) {
    if (!rect || !vp || !sc || !shift || rw <= 0 || rh <= 0 || rect[2] <= 0 || rect[3] <= 0 ||
        !(vp[2] > 0) || !(vp[3] > 0) || !isfinite(vp[0]) || !isfinite(vp[1]) || !isfinite(vp[2]) || !isfinite(vp[3])) return 0;
    // The guest may use a full-screen or UI-band viewport. Scissors remain in
    // the unshifted UI band; dividing its height by the full portrait height
    // rejects the real map scissor and silently leaves it below screen centre.
    double sx = vp[2] / rw;
    double ys[2] = {vp[3] / rh, 0};
    if (isfinite(ui_hfrac) && ui_hfrac > 0 && ui_hfrac < 1)
        ys[1] = vp[3] / (rh * ui_hfrac);
    for (int basis = 0; basis < 2; basis++) {
        double sy = ys[basis]; if (!(sy > 0)) continue;
        double want[4] = {vp[0] + rect[0]*sx, vp[1] + rect[1]*sy, rect[2]*sx, rect[3]*sy};
        int match = 1;
        for (int i = 0; i < 4; i++) {
            double tolerance = 2 * fmax(1, i == 0 || i == 2 ? sx : sy);
            if (fabs(sc[i] - want[i]) > tolerance) { match = 0; break; }
        }
        if (!match) continue;
        *shift = lround(dy * sy);
        return 1;
    }
    return 0;
}
