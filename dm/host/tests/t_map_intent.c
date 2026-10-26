#include "../dm_map_intent.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
    dm_map_intent s={-1,-1};
    assert(dm_map_intent_select(&s,2,-1,0)==2);
    assert(dm_map_intent_select(&s,2,1,0)==0);
    assert(dm_map_intent_select(&s,2,-1,1)==2);
    assert(dm_map_intent_select(&s,2,-1,0)==0); // Close panel restores selection.
    assert(dm_map_intent_select(&s,2,-1,0)==0); // New area retains choice.
    assert(dm_map_intent_select(&s,2,0,0)==2);
    assert(dm_map_intent_select(&s,0,-1,0)==0); // Full-screen preference is respected.
    assert(dm_map_intent_select(&s,0,-1,1)==2);
    assert(dm_map_intent_select(&s,0,-1,0)==0);
    assert(dm_map_intent_select(&s,0,0,0)==2);
    assert(dm_map_intent_select(&s,1,-1,0)==1); // Changing preference replaces temporary choice.
    assert(dm_map_intent_select(&s,1,1,1)==1);
    assert(dm_map_intent_select(&s,1,-1,0)==0);
    s=(dm_map_intent){-1,-1}; assert(dm_map_intent_select(&s,1,-1,0)==1);
    puts("PASS map intent: panel hold, area reapply, manual dismissal, original full/left/right preferences and new session");
}
