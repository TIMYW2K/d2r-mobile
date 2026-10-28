// Exercise the actual per-scene policy without starting the game or injecting any input.
#include "../dm_minimap_policy.h"
#include <stdio.h>

static int checks, fails;
#define CHECK(c, what) do { ++checks; if (!(c)) { ++fails; fprintf(stderr, "FAIL: %s\n", what); } } while (0)
static int step(dm_minimap_entry *s, uint64_t frame, int ready, int area, int shown) {
    return dm_minimap_entry_step(s, frame, ready, area, shown);
}
int main(void) {
    dm_minimap_entry s = {0};
    CHECK(dm_minimap_recovery_ready(1,1,0,1,1,0,0), "ready touch game with auto map");
    CHECK(!dm_minimap_recovery_ready(1,1,0,1,1,1,0), "never toggle during cinematics");
    CHECK(!dm_minimap_recovery_ready(1,1,0,1,0,0,0), "auto-open disabled stays disabled");
    CHECK(!dm_minimap_recovery_ready(1,1,1,1,1,0,0), "no toggle during loading");
    CHECK(!dm_minimap_recovery_ready(1,1,0,1,1,0,1), "controller mode retains manual control");
    int fired = 0;
    // Readiness represents all integration gates: ingame, valid player, load<=0, live AutoMap/vtable.
    for (int f = 0; f < 20; ++f) fired += step(&s, f, 0, 1, 0);
    CHECK(!fired && !s.active && !s.attempts, "unready old scene never requests an open");
    CHECK(!step(&s, 20, 1, 1, 0), "first ready frame begins a grace period");
    CHECK(!step(&s, 25, 1, 1, 0), "wait for delayed map initialization");
    CHECK(step(&s, 26, 1, 1, 0), "first retry after six ready frames");
    CHECK(!step(&s, 26, 1, 1, 0), "same-frame repeated observation cannot inject twice");
    CHECK(s.attempts == 1, "first retry counted once");
    CHECK(!step(&s, 27, 1, 1, 1), "late-visible map completes recovery without another toggle");
    CHECK(!s.pending, "late visibility disarms recovery");
    for (int f = 28; f < 200; ++f) fired += step(&s, f, 1, 1, 0);
    CHECK(!fired && s.attempts == 1, "user close after visibility stays respected in the same scene");

    CHECK(!step(&s, 200, 1, 2, 0), "new area arms its own grace period");
    CHECK(s.area == 2 && s.attempts == 0 && s.pending, "new area resets retry budget");
    fired = 0;
    for (int f = 201; f <= 400; ++f) {
        int fire = step(&s, f, 1, 2, 0);
        if (fire) CHECK(f == 206 || f == 236 || f == 266, "retry cadence is bounded and spaced");
        fired += fire;
    }
    CHECK(fired == 3 && s.attempts == 3 && !s.pending, "per-scene recovery stops after three failed retries");
    CHECK(!step(&s, 1000, 1, 2, 0), "expired retry budget does not toggle forever");

    CHECK(!step(&s, 1001, 0, 2, 0), "loading transition injects no map key");
    CHECK(!s.active, "loading disarms the old scene");
    CHECK(!step(&s, 1010, 1, 2, 0), "reentering same area after loading starts fresh grace");
    CHECK(step(&s, 1016, 1, 2, 0), "same-area reentry can recover its first invisible map");
    CHECK(s.attempts == 1, "same-area load reentry has a fresh retry budget");
    CHECK(!step(&s, 1017, 1, 2, 1), "successful reentry display cancels future retries");
    CHECK(!step(&s, 1117, 1, 2, 0), "closing recovered reentry map stays respected");

    CHECK(!step(&s, 1120, 1, 3, 1), "already-visible new-area map is never toggled closed");
    CHECK(!s.pending && s.attempts == 0, "already-visible entry needs no retry");
    CHECK(!step(&s, 1200, 1, 3, 0), "manual close of initially visible map stays respected");
    // A story movie resets the scene latch even if the next act reuses the same area id.
    CHECK(!step(&s, 1201, dm_minimap_recovery_ready(1,1,0,1,1,1,0), 3, 0), "movie blocks input and rearms");
    CHECK(!step(&s, 1202, dm_minimap_recovery_ready(1,1,0,1,1,0,0), 3, 0), "movie completion starts a new grace period");
    CHECK(step(&s, 1208, dm_minimap_recovery_ready(1,1,0,1,1,0,0), 3, 0), "map reopens after movie ends");
    CHECK(!step(&s, 1209, 1, 0, 0) && !s.active, "unknown area cannot request a key");
    CHECK(!step(&s, 1210, 1, 4, 0), "new valid area begins grace after unknown-area interval");
    CHECK(!step(&s, 1216, 0, 4, 0), "readiness loss at retry deadline prevents input");
    CHECK(!step(&s, 1220, 1, 4, 0), "readiness recovery uses a new grace period");
    CHECK(step(&s, 1226, 1, 4, 0), "retry resumes only for ready current scene");
    printf("minimap policy: %d checks, %d failures\n", checks, fails);
    return fails != 0;
}
