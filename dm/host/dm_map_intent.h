#pragma once
typedef struct { int cfg_seen,want; } dm_map_intent;
static inline int dm_map_intent_select(dm_map_intent *s,int cfg,int request,int hold) {
    if (cfg!=s->cfg_seen) { s->cfg_seen=cfg; s->want=cfg==0; }
    if (request>=0) s->want=!!request;
    return s->want && !hold ? 0 : cfg==1 ? 1 : 2;
}
