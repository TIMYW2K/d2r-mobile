"""Native VP8 movies for this iOS build; DM_MOVIE_FFMPEG=0 disables explicitly."""
import os
def enabled():
    return os.environ.get('DM_MOVIE_FFMPEG',os.environ.get('MOVIE_FFMPEG','1')) == '1'
