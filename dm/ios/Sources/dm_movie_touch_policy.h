#pragma once
// Credits have a background video, but their native Exit button owns touches.
static inline int dm_movie_touch_allowed(int decoding,int credits_visible) {
    return decoding && !credits_visible;
}
