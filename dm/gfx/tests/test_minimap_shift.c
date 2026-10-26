#include "../nvn/nvn_minimap_shift.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
    const int rect[4] = {402, 48, 402, 444};
    const double native[4] = {0,0,1206,2622};
    const int sc[4] = {402,48,402,444}; long shift=0;
    assert(nvn_minimap_scissor_shift(rect,1206,2622,native,sc,-1144,&shift) && shift == -1144);
    // 1288 UI downshift + native y48 -1144 = desired screenshot y192.
    assert(1288 + sc[1] + shift == 192);
    const double half[4] = {0,0,603,1311}; const int half_sc[4] = {201,24,201,222};
    assert(nvn_minimap_scissor_shift(rect,1206,2622,half,half_sc,-1144,&shift) && shift == -572);
    const int other[4] = {0,0,1206,2622};
    assert(!nvn_minimap_scissor_shift(rect,1206,2622,native,other,-1144,&shift));
    const int rounded[4] = {403,49,401,445};
    assert(nvn_minimap_scissor_shift(rect,1206,2622,native,rounded,-1144,&shift));
    const int wrong[4] = {402,52,402,444};
    assert(!nvn_minimap_scissor_shift(rect,1206,2622,native,wrong,-1144,&shift));
    assert(!nvn_minimap_scissor_shift(rect,0,2622,native,sc,-1144,&shift));
    puts("PASS minimap native/scaled scissor matching, rounding, unrelated draw rejection and target y192");
    return 0;
}
