#define DM_MOVIE_FFMPEG 1
#include "../hle.h"
#undef HLE_FORCE
#define HLE_FORCE(mangled,why,ret,cname,...) static ret cname(__VA_ARGS__)
#include "../hle_movie.c"
#include <assert.h>
#include <stdio.h>
void dm_movie_decode_close(dm_movie_decode *decode) { assert(decode==(dm_movie_decode *)1); }
int main(void) {
    assert(!movie_is_cutscene(NULL));assert(!movie_is_cutscene("data/LOCAL/LogoLoop.webm"));
    assert(!movie_is_cutscene("logoloop"));assert(movie_is_cutscene("data/movies/act1.webm"));
    mvreal a={.decode=(dm_movie_decode *)1,.cutscene=1},b={.decode=(dm_movie_decode *)1,.cutscene=1};
    mvreal background={.decode=(dm_movie_decode *)1,.cutscene=0},missing={.cutscene=1};
    assert(!dm_movie_cutscene_active());movie_started(&background,1);movie_started(&missing,1);
    assert(!dm_movie_cutscene_active());movie_started(&a,1);movie_started(&a,1);
    assert(dm_movie_cutscene_active() && atomic_load(&movie_cutscenes)==1);
    movie_started(&b,1);movie_started(&a,0);assert(dm_movie_cutscene_active());
    movie_started(&a,0);movie_started(&b,0);assert(!dm_movie_cutscene_active());
    movie_started(&background,0);movie_started(&missing,0);assert(!atomic_load(&movie_cutscenes));
    mvreal *released=calloc(1,sizeof *released);released->refs=1;released->cutscene=1;released->decode=(dm_movie_decode *)1;
    void *self[2]={0,released};movie_started(released,1);assert(dm_movie_cutscene_active());
    movie_release(self);assert(!self[1] && !dm_movie_cutscene_active());
    puts("movie touch state: background/missing excluded; start/stop idempotent; concurrent ownership passed");return 0;
}
