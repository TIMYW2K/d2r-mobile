// Synthetic stand-in for dm_vault_tab.h (generated from the player's own RomFS by
// dm/ios/tools/gen_vault_table.py, therefore not checked in).
// t_vault_icons only exercises vu_tile with synthetic rows; it never walks the real
// table, so a tiny fixture is enough to link and run the test without game data.
// When a real dm_vault_tab.h exists next to dm_vault_ui.c, the compiler picks that
// one first and this fixture is ignored.
#ifndef DM_VAULT_TAB_STUB_H
#define DM_VAULT_TAB_STUB_H
#include <stdint.h>

#define VT_NPART 2
static const char *const vt_part_name[VT_NPART] = { "Test A", "Test B" };

typedef struct { uint8_t kind, part, tier; uint16_t row, lvl; int16_t set; const char *key, *icon; } vt_row;
static const vt_row vt_rows[] = {
    { 0, 0, 0, 0, 1, 0, "Test Set Item", "Items/armor/helmet/cap" },
    { 0, 1, 0, 1, 1, 1, "Test Set Piece", "Items/weapon/sword/short_sword" },
    { 1, 0, 0, 2, 1, -1, "Test Unique", "Items/misc/ring/ring" },
};
#define VT_NROW (int)(sizeof vt_rows / sizeof vt_rows[0])

static const char *const vt_set_key[] = { "Test Set A", "Test Set B" };
#define VT_NSET (int)(sizeof vt_set_key / sizeof vt_set_key[0])
#endif
