# Experimental 60 Hz loot labels

Status: experimental renderer, not yet validated on a device. The game thread
captures owned snapshots and a separate display link requests 60 Hz. Simple,
single-line labels can be drawn with the bundled game font; multiline and inline
color-code labels stay native. Original drawing remains the fallback when the
gate, frame, text, colors, location, scene, focus or presentation acknowledgment
does not match. This request can affect power use and does not guarantee actual
60 Hz presentation. Verify appearance, placement and pickup on an iPhone.

For the current 3.3 image, the label-manager entry main+0xc017a0 calls
main+0xc0189c to rebuild the item-label records. Each record has stride 0x148;
the existing world-tap path uses its rectangle at +0 and item ID at +0x10.
The builder call at main+0xc042d0 passes record+0x24 to main+0x43b1e0.
At main+0x43b214 the capacity argument passed to the name builder is 0x80;
main+0x43b22c invokes main+0x455840 to write the text to that buffer.
The prototype makes a bounded, validated owned UTF-8 copy of that inline text;
the text encoding and runtime color markup still require a device check.
After records are prepared, main+0xc0187c is the only direct call to
main+0x43b850. At the call x0 points at the label record, x1 at record+0x24,
and x2 at record+0x14. The callee invokes separate background and text drawing
functions. This is a candidate for separating the draw without suppressing
the construction of labels; it is not proof that captured object references
or font resources remain valid across frames.

Run python -X utf8 dm/ios/tests/test_loot_draw_contract.py <IPA> before using
these offsets. The check validates instructions, branch targets, and all
direct BL references in the current image. It does not validate indirect calls, pointers, text encoding, color markup,
the render command stream, reentrancy, or ABI behavior on a device. The
experimental rewrite routes the one draw call through svc 0x8f, which forwards
to the native draw unless an eligible overlay frame was acknowledged.

dm_loot_capture.h makes a bounded copy of record text, colors and geometry.
Opaque record bytes may contain pointers: copying them does not extend their
lifetime and they must not be dereferenced on another thread. A real pipeline
needs a validated owned representation or a GPU resource lifetime protocol,
an independent 60 Hz presentation path, a fallback when any required state is
unavailable, and an explicit handshake before suppressing the original draw.
Display and pickup must use the same geometry snapshot. On background, loading,
rotation, scene changes, disappearances, and camera discontinuities, discard
stale data and restore the native draw.

The experimental dm_loot_exchange.c bridge holds owned current/previous
snapshots behind a mutex. A nonblocking reader receives a copy and an epoch;
scene or orientation invalidation changes the epoch, while stale snapshots are
rejected. A sampled frame plan shares one rectangle set for display and hit
testing. UIKit acknowledges a completed draw on the next display-link tick;
this is not a GPU presentation fence and must be checked on real devices.
The game draw gate suppresses a native label only if its ID, text, colors,
position, epoch and age match an eligible acknowledgment; otherwise it forwards
the native call. Unknown or unsupported text stays native.

The existing runtime world extrapolator requires a 60 Hz target and was disabled
after real-device stutter/ghosting. It is not a tested 30-to-60 loot solution.
The frame target can also be limited by iOS, power/thermal state, or available
hardware: a 60 Hz request cannot guarantee 60 actual presentations.
