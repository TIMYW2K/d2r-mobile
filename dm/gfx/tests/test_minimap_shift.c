#include "../nvn/nvn_minimap_shift.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
    const int rect[4] = {402, 48, 402, 444};
    const double native[4] = {0,0,1206,2622};
    const int sc[4] = {402,48,402,444}; long shift=0;
    assert(nvn_minimap_scissor_shift(rect,1206,2622,native,sc,1,-1144,&shift) && shift == -1144);
    // 1288 UI downshift + native y48 -1144 = desired screenshot y192.
    assert(1288 + sc[1] + shift == 192);
    const double half[4] = {0,0,603,1311}; const int half_sc[4] = {201,24,201,222};
    assert(nvn_minimap_scissor_shift(rect,1206,2622,half,half_sc,1,-1144,&shift) && shift == -572);
    const int other[4] = {0,0,1206,2622};
    assert(!nvn_minimap_scissor_shift(rect,1206,2622,native,other,1,-1144,&shift));
    const int rounded[4] = {403,49,401,445};
    assert(nvn_minimap_scissor_shift(rect,1206,2622,native,rounded,1,-1144,&shift));
    const int wrong[4] = {402,52,402,444};
    assert(!nvn_minimap_scissor_shift(rect,1206,2622,native,wrong,1,-1144,&shift));
    assert(!nvn_minimap_scissor_shift(rect,0,2622,native,sc,1,-1144,&shift));
    // Actual portrait UI is a 1334px bottom band within the 2622px screen.
    // Its viewport height can be the band itself, or a scaled band. Both must
    // retain the map's native clip and reach the same red-box target.
    const double band[4] = {0,0,1206,1334};
    assert(nvn_minimap_scissor_shift(rect,1206,2622,band,sc,1334.0/2622,-1144,&shift) && shift==-1144);
    assert(1288 + sc[1] + shift == 192);
    const double band_half[4] = {0,0,603,667};
    assert(nvn_minimap_scissor_shift(rect,1206,2622,band_half,half_sc,1334.0/2622,-1144,&shift) && shift==-572);
    assert(!nvn_minimap_scissor_shift(rect,1206,2622,band,other,1334.0/2622,-1144,&shift));
    puts("PASS minimap native/scaled scissor matching, rounding, unrelated draw rejection and target y192");
    return 0;
}
