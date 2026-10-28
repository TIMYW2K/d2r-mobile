// 「自定义」设置：我们自己加的功能的启停开关。接口见 dm_custom.h。
// 存储：存档提交目录旁的 sw_custom.txt（iOS 上在 Documents 里，升级包不丢），一行一个「键=0/1」；没有这个文件 / 没写的键 = 各键缺省（其余开）；不认识的行（旧版的 drop_filter、ver、eco_fps）读时跳过。
// 掉落筛选照红叶 mod 一件都不藏，整块拆掉，drop_filter 键与它的 ver=2 迁移一起删了
// 不写进游戏自己的设置存档：不依赖它的格式与写盘时机，应用 / 放弃 / 默认由我们自己按原版选项页的语义做。
// 界面：选项面板「自定义」页（借原版视频页 VideoOptionsPanel），每行一个原版勾选框，控件名 TouchCustom_<键>（布局 touch_layout.py 的 _custom_page）。勾选只记成待定值，
// 「应用」（OptionsPanelMessage:AcceptChanges）才生效并存盘，「放弃」（CancelChanges）丢掉待定值，「默认」（ResetToDefaults）待定值回各键缺省（其余开；还要点应用）
// 「自定义」(Custom) settings: on/off switches for the features we added ourselves. For the interface see dm_custom.h.
// Storage: sw_custom.txt next to the save commit directory (on iOS it is in Documents and survives app upgrades), one 'key=0/1' per line; without this file / for keys not written = each key's default (power-saving mode off, the rest on); unrecognized lines (drop_filter and ver from old versions) are skipped when reading.
// Drop filtering follows the Hongye community mod and hides nothing, so the whole block was removed, and the drop_filter key was deleted together with its ver=2 migration
// Not written into the game's own settings save: no dependence on its format or on when it writes to disk; apply / discard / default are done by ourselves following the semantics of the original game's options page.
// UI: the 「自定义」(Custom) page of the options panel (borrowing the original game's video page VideoOptionsPanel), one original-game checkbox per row, widget name TouchCustom_<key> (_custom_page in the layout touch_layout.py). Checking only records a pending value;
// only 「应用」(Apply) (OptionsPanelMessage:AcceptChanges) makes it take effect and saves to disk, 「放弃」(Discard) (CancelChanges) throws away the pending values, and 「默认」(Default) (ResetToDefaults) sets the pending values back to each key's default (power-saving mode off, the rest on; Apply still has to be tapped)
#include "dm_custom.h"
#include "dm_widget.h"
#include "dm_cursor.h"
#include "hle.h"
#include "dm_input.h"
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static const char *const k_keys[] = { "lang_en", "auto_mana", "merc_pot", "drop_light", "map_mark", "item_notes", "icloud", "pot_pick", "drop_haptic", "gem_pick", "store_btn", "q3d", "loot_filter", "auto_buff", "touch_swap_above", "hdr", "zoom", "metalfx", "cam", "show_fps" };   // 顺序 = 自定义页从上到下的行序（touch_layout.py _CUSTOM_ROWS；show_fps 不在自定义页，排末尾、只走游戏设置页）。T18：省电模式（eco_fps）已删除，帧率只由温控决定 | order = the custom page's top-to-bottom rows (touch_layout.py _CUSTOM_ROWS; show_fps is not on the custom page, it is last and only lives on the game's settings page). T18: the power-saving mode (eco_fps) is gone; the frame rate is only capped by the thermal state
#define NK (int)(sizeof k_keys / sizeof *k_keys)   // 行数（解冲突时漏掉过，这里补回）
static const int k_def[NK] = { 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 0, 0, 1, 1, 0, 0 };   // 缺省值：性能模式流畅优先（q3d=0）、touch_swap_above=0（传送门左侧）、hdr=0、show_fps=0，其余开；zoom=1（允许双指缩放） | defaults: performance mode smoothness first (q3d=0), touch_swap_above=0 (left of portal), hdr=0, show_fps=0, the rest on; zoom=1 (pinch-to-zoom allowed)
// q3d（性能模式，自定义页）：0 = 流畅优先（60 帧，掉帧就自动降 3D 比例）、1 = 画质优先（60 帧，不自动降，过热仍会自动降档）、
// 2 = 画质优先（强制）（60 帧固定、固定 3D 画质，忽略过热自动降档——永不 30 帧、永不降 3D 比例）。生效 = 调 hle_oe.c 的
// dm_3d_quality_mode_set，读盘与点「应用」时当场调，下一帧控制器就按新档走；q3d==2 时 dm_fps_setting 恒 60、并立刻清掉已有温控降档。
// 页上是上下三项单选（同语言行）：勾选框 TouchCustom_q3d_smooth（流畅）/ TouchCustom_q3d（画质）/ TouchCustom_q3d_force（强制），三框互斥。
// q3d (performance mode, custom page): 0 = smoothness first (60 fps, automatically lowers the 3D scale on frame drops),
// 1 = quality first (60 fps, no automatic lowering; the thermal protection may still lower it), 2 = quality first (forced):
// fixed 60 fps + fixed 3D quality, ignoring the automatic thermal downgrade (never 30 fps, never lowers the 3D scale).
// Applied by calling hle_oe.c's dm_3d_quality_mode_set on load / on tapping "Apply". When q3d == 2 dm_fps_setting always
// returns 60 and any active thermal downgrade is cleared at once. On the page it is a stacked three-option radio (like the
// language row): checkboxes TouchCustom_q3d_smooth (smoothness) / TouchCustom_q3d (quality) / TouchCustom_q3d_force (forced),
// three-way mutually exclusive.
#define K_Q3D 11
#define K_TOUCH_SWAP 14
#define K_SHOW_FPS (NK - 1)   // show_fps 排在键表末尾、只在游戏设置页有勾选框 | show_fps is last in the key table and only has a checkbox on the game's settings page
#define K_PAGE_ROWS (NK - 1)  // 自定义页的行数 = 除 show_fps 外的全部键 | rows on the custom page = every key except show_fps
#define K_CAM (NK - 2)
#define CAM_N 3
static const char *const k_cam_box[CAM_N] = { "TouchCustom_cam", "TouchCustom_cam_ots", "TouchCustom_cam_fp" };
static const char *const k_cam_name[CAM_N] = { "off", "over-the-shoulder", "first-person" };
void dm_cam_set_mode(int m);
void dm_3d_quality_mode_set(int mode);
static const char *q3d_name(int m) { return m >= 2 ? "画质优先（强制）" : m == 1 ? "画质优先" : "流畅优先"; }   // 三档名字 | the three mode names
static const char *const q3d_box[3] = { "TouchCustom_q3d_smooth", "TouchCustom_q3d", "TouchCustom_q3d_force" };   // 勾选框（值 0/1/2） | checkboxes (values 0/1/2)
static const char *rev_box(int i) { return i == 0 ? "TouchCustom_lang_zh" : i == K_TOUCH_SWAP ? "TouchCustom_touch_swap_above_near" : NULL; }   // 二选行「反面那一项」（q3d 是三选行，单独处理） | inverse checkbox of a two-option radio (q3d is a three-option radio and is handled separately)   // 单选行「反面那一项」的勾选框 | Checkbox of the radio row's "inverse option"
static _Atomic int g_val[NK];        // 已生效：1 开 / 0 关 | In effect: 1 on / 0 off
static _Atomic int g_pend[NK];       // 选项页上改了还没应用：-1 = 没改 | Changed on the options page but not applied yet: -1 = not changed
static _Atomic int g_loaded;         // 0 没读盘 / 1 已读 | 0 not read from disk / 1 read
// lang_en（语言开关，自定义页第一行）：勾上 = 英文界面 + 英文语音。它不是功能开关，生效方式不同——下次启动挂存档时由 hle_fs.c 按它改写原版设置文件的
// Language / VoiceLanguage（游戏运行中自己会把内存里的语言写回设置文件，所以只能在游戏读设置之前改；见 dm_custom_lang_explicit）。
// 文件里没有这个键（老玩家、还没点过应用）= 不动原版设置，勾选框按当前实际语言显示；「默认」不改它（按一下默认把语言切了不合适）
// lang_en (language switch, first row of the Custom page): checked = English UI + English voice. It is not a feature switch and takes effect differently -- at the next launch, when the save is mounted, hle_fs.c rewrites the original game's settings file's
// Language / VoiceLanguage according to it (while running, the game itself writes the in-memory language back to the settings file, so it can only be changed before the game reads its settings; see dm_custom_lang_explicit).
// Key absent from the file (an existing player who has not yet tapped Apply) = the original settings are left alone, and the checkbox shows the actual current language; 「默认」(Default) does not change it (switching the language at one press of Default would be inappropriate)
static _Atomic int g_lang_explicit = -1;   // 文件里写着的 lang_en：-1 没写 / 0 / 1 | lang_en as written in the file: -1 not written / 0 / 1
// merc_pot（佣兵自动喂药）原先缺省关、现在缺省开。旧文件每次存盘都把全部键写一遍，里面的 merc_pot=0 分不出「没动过」还是「自己关的」，
// 所以另记一行 merc_pot_v2：只有在缺省改成开之后玩家自己在页上改过这一项才写。读盘时有 merc_pot_v2 以它为准；没有 = 没在新缺省下动过 → 开
// （旧文件里的 merc_pot=1 也是开，结果一样）。merc_pot 那一行照旧写当前值，旧版本的包读它不出错
// merc_pot (mercenary auto-feed potions) used to be off by default and is now on by default. Old files write all keys on every save, and the merc_pot=0 in them cannot tell "never touched" from "turned off by the player",
// so a separate line merc_pot_v2 is recorded: it is written only when the player has changed this item on the page after the default was changed to on. On load, if merc_pot_v2 exists it is authoritative; absent = never touched under the new default → on
// (merc_pot=1 in an old file is also on, the same result). The merc_pot line is still written with the current value, so packages of older versions read it without error
static _Atomic int g_merc_explicit = -1;   // 文件里写着的 merc_pot_v2：-1 没写 / 0 / 1 | merc_pot_v2 as written in the file: -1 not written / 0 / 1
#define K_MERC 2
int dm_lang_zh(void);
static _Atomic int g_repaint;        // 默认 / 放弃之后：页上的勾选框按待定值 / 生效值重摆一遍 | After default / discard: lay out the checkboxes on the page again according to pending values / effective values

const char *dm_save_commitdir(void);
static int key_index(const char *k) { if (k) for (int i = 0; i < NK; i++) if (!strcmp(k, k_keys[i])) return i; return -1; }
static char g_dir[1024];   // iOS 外壳启动时给的 Documents（main.m）：启动期（存档挂上之前）就要能读，覆盖层选择用 | Documents given by the iOS shell at startup (main.m): must be readable already during startup (before the save is mounted), used for overlay selection
void dm_custom_init(const char *dir) { if (dir) strlcpy(g_dir, dir, sizeof g_dir); }
static int file_path(char *p, size_t n) {
    if (g_dir[0]) { snprintf(p, n, "%s/sw_custom.txt", g_dir); return 1; }
    const char *c = dm_save_commitdir(); if (!c) return 0; snprintf(p, n, "%s/../sw_custom.txt", c); return 1; }

static void load_once(void) {
    if (atomic_load(&g_loaded)) return;
    char p[1100]; if (!file_path(p, sizeof p)) return;   // 存档还没挂上：先按缺省开答，挂上后再读 | The save is not mounted yet: answer with the default (on) for now, and read again after it is mounted
    int v[NK]; for (int i = 0; i < NK; i++) v[i] = k_def[i];
    FILE *f = fopen(p, "r"); char ln[128];
    int lang = -1, merc = -1;
    // q3d 是 0/1/2：这一项按数字读（其余键仍是 0/1） | q3d is 0/1/2: this key is parsed as a number (the other keys stay 0/1)
    while (f && fgets(ln, sizeof ln, f)) { char *eq = strchr(ln, '='); if (!eq) continue; *eq = 0; int i = key_index(ln); if (i >= 0) v[i] = (i == (int)K_Q3D || i == (int)K_CAM) ? atoi(eq + 1) : eq[1] != '0'; if (i == 0) lang = v[0]; if (!strcmp(ln, "merc_pot_v2")) merc = eq[1] != '0'; }
    if (v[K_CAM] < 0 || v[K_CAM] >= CAM_N) v[K_CAM] = 0;
    if (f) fclose(f);
    if (lang < 0) v[0] = !dm_lang_zh();   // 没写过：按当前实际语言显示 | Never written: display according to the current actual language
    atomic_store(&g_lang_explicit, lang);
    v[K_MERC] = merc < 0 ? k_def[K_MERC] : merc; atomic_store(&g_merc_explicit, merc);
    if (v[K_Q3D] < 0) v[K_Q3D] = 0;
    if (v[K_Q3D] > 2) v[K_Q3D] = 2;   // 迁移：旧配置只有 0/1，1 保持画质优先 | migration: old files only have 0/1; 1 stays quality first
    for (int i = 0; i < NK; i++) { atomic_store(&g_val[i], v[i]); atomic_store(&g_pend[i], -1); }
    dm_cam_set_mode(v[K_CAM]);
    dm_3d_quality_mode_set(v[K_Q3D]); LOG("自定义设置：性能模式读盘为%s（q3d=%d，已交给 3D 比例控制）", q3d_name(v[K_Q3D]), v[K_Q3D]);
    if (v[K_Q3D] == 2) { dm_perf_thermal_set(0); LOG("自定义设置：强制画质模式（q3d=2）→ 固定 60 帧 / 固定 3D 画质，忽略过热降档"); }
    if (!atomic_exchange(&g_loaded, 1)) LOG("自定义设置：读盘 %s%s lang_en=%d%s auto_mana=%d merc_pot=%d drop_light=%d map_mark=%d item_notes=%d icloud=%d pot_pick=%d drop_haptic=%d gem_pick=%d store_btn=%d show_fps=%d", p, f ? "" : "（没有文件，全按缺省）", v[0], lang < 0 ? "（没写，按当前语言）" : "", v[1], v[2], v[3], v[4], v[5], v[6], v[7], v[8], v[9], v[10], v[K_SHOW_FPS]);
}
int dm_custom_get(const char *key) {
    load_once(); int i = key_index(key); if (i < 0) return 1;
    return atomic_load(&g_loaded) ? atomic_load(&g_val[i]) : k_def[i];
}
int dm_custom_lang_explicit(void) { load_once(); return atomic_load(&g_loaded) ? atomic_load(&g_lang_explicit) : -1; }
// 本地改动：温控自动降档（T8，无 UI）。前端 front_ios.m 读 NSProcessInfo.thermalState：serious → 1、critical → 2，
// 回落 < serious 并保持 60 秒后回 0；这里只存档位并提供运行期帧率上限，3D 比例温控下限在 hle_oe.c 用同一档位实现。
// T18：省电模式删除后，帧率只剩这一道上限（正常 60 / 温热 30）；画质优先也不影响过热保护。
// Local change: automatic thermal downgrade (T8, no UI). The front end maps NSProcessInfo.thermalState (serious -> 1,
// critical -> 2) and returns to 0 only after 60 s below serious; this file only stores the level and exposes the runtime
// frame-rate cap, while hle_oe.c applies the 3D scale floor from the same level. T18: with the power-saving mode removed
// this is the only frame-rate cap (60 normal / 30 warm); "quality first" does not disable the thermal protection.
static _Atomic int g_thermal_level;   // 0 正常 / 1 严重 / 2 危急 | 0 normal / 1 serious / 2 critical
void dm_perf_thermal_set(int level) {
    if (level < 0) level = 0;
    if (level > 2) level = 2;
    int old = atomic_exchange(&g_thermal_level, level);
    if (old == level) return;
    LOG("温控优化：热档位 %d → %d（%s），帧率上限 %d，3D 比例温控下限第 %d 档",
        old, level, level >= 2 ? "危急" : level == 1 ? "严重" : "正常", level ? 30 : 60, level >= 2 ? 4 : 3);
}
int dm_perf_thermal_level(void) { return atomic_load(&g_thermal_level); }
int dm_3d_profile_get(void) { return dm_custom_get("metalfx") && dm_custom_get("q3d") != 2 ? 0 : 4; }
int dm_fps_setting(void) {   // 帧率上限：温控决定，但性能模式「画质优先（强制）」q3d=2 时恒 60（忽略温控） | frame-rate cap: thermal, except performance mode q3d=2 (forced quality) which is always 60
    if (atomic_load(&g_val[K_Q3D]) == 2) return 60;
    return dm_perf_thermal_level() ? 30 : 60;
}   // 图形层 nvn_mtl.m mtl_fps_target 每帧读（弱符号的强定义） | Read every frame by mtl_fps_target in the graphics layer nvn_mtl.m (strong definition of a weak symbol)
static void save(void) {
    char p[1100], t[1120]; if (!file_path(p, sizeof p)) { LOG("【红】自定义设置：存档目录没挂上，没存盘"); return; }
    snprintf(t, sizeof t, "%s.tmp", p); FILE *f = fopen(t, "w"); int ok = f != NULL;
    for (int i = 0; ok && i < NK; i++) { if (i == 0 && atomic_load(&g_lang_explicit) < 0) continue; ok = fprintf(f, "%s=%d\n", k_keys[i], atomic_load(&g_val[i])) > 0; }   // lang_en 只在玩家真改过之后才写进文件 | lang_en is written to the file only after the player has actually changed it
    if (ok && atomic_load(&g_merc_explicit) >= 0) ok = fprintf(f, "merc_pot_v2=%d\n", atomic_load(&g_merc_explicit)) > 0;
    if (f) ok = fclose(f) == 0 && ok;
    if (ok) ok = rename(t, p) == 0;   // 先写临时文件再换名：写到一半被杀也不会留下半个文件 | Write a temporary file first and then rename: being killed halfway through the write does not leave half a file behind
    LOG("自定义设置：存盘%s", ok ? "" : "【红】失败");
}
void dm_custom_set_live(const char *key, int val) {
    load_once();
    int i = key_index(key);
    if (i >= 0) {
        int old = atomic_exchange(&g_val[i], val != 0);
        if (old != (val != 0)) {
            save();
            LOG("自定义设置（实时）：%s → %s 并存盘", key, val ? "开" : "关");
        }
    }
}
void dm_custom_on_uimsg(const char *cat, const char *act, const char *arg) {
    (void)arg; if (!cat || !act || strcmp(cat, "OptionsPanelMessage")) return;
    load_once();
    if (!strcmp(act, "AcceptChanges")) { int ch = 0;
        for (int i = 0; i < NK; i++) { int pd = atomic_exchange(&g_pend[i], -1); if (pd >= 0 && pd != atomic_load(&g_val[i])) { atomic_store(&g_val[i], pd); ch = 1; if (i == K_CAM) dm_cam_set_mode(pd); if (i == K_Q3D) { int dm_3d_quality_mode_get(void); float dm_3d_scale_cur(void); dm_3d_quality_mode_set(pd);
            if (pd == 2) { dm_perf_thermal_set(0); LOG("自定义设置：强制画质模式（q3d=2）→ 固定 60 帧 / 固定 3D 画质，忽略过热降档"); }
            LOG("自定义设置：性能模式 → %s（已交给 3D 比例控制，读回模式 %d，此刻 3D 比例 %.2f）", q3d_name(pd), dm_3d_quality_mode_get(), dm_3d_scale_cur()); } if (i == 0) atomic_store(&g_lang_explicit, pd); if (i == K_MERC) atomic_store(&g_merc_explicit, pd); LOG("自定义设置：%s → %s（应用%s）", k_keys[i], pd ? "开" : "关", i == 0 ? "；语言下次启动生效" : ""); } }
        if (ch) save(); }
    else if (!strcmp(act, "CancelChanges")) { for (int i = 0; i < NK; i++) atomic_store(&g_pend[i], -1); atomic_store(&g_repaint, 1); }
    else if (!strcmp(act, "ResetToDefaults")) { for (int i = 1; i < K_PAGE_ROWS; i++) atomic_store(&g_pend[i], k_def[i]); atomic_store(&g_repaint, 1); }   // 从 1 起：语言那一格不归「默认」管；只到自定义页最后一行（show_fps 不在这页，不跟着重置） | Starts from 1: the language cell is not governed by 「默认」(Default); ends at the custom page's last row (show_fps is not on this page, so it is not reset)
}

// ---- 选项页上的勾选框
// ---- Checkboxes on the options page
static uint8_t *main_base(void) { static uint8_t *b; if (!b) for (int i = 0; i < g_nmod; i++) if (!strcmp(g_mod[i].name, "main")) b = (uint8_t *)g_mod[i].base; return b; }
static uint64_t find_w(uint64_t w, const char *name, int dep) {   // 按名字找控件：名字 +8、子控件数组 +0x58、个数 +0x60 | find a widget by name: name +8, child widget array +0x58, count +0x60
    if (!w || dep > 14) return 0; const char *nm = (const char *)*(uint64_t *)(w + 8); if (nm && !strcmp(nm, name)) return w;
    uint64_t ch = *(uint64_t *)(w + 0x58), n = *(uint64_t *)(w + 0x60); if (!ch || n > 256) return 0;
    for (uint64_t i = 0; i < n; i++) { uint64_t r = find_w(*(uint64_t *)(ch + 8 * i), name, dep + 1); if (r) return r; }
    return 0;
}
static uint64_t find_w_off(uint64_t w, const char *name, int dep) {   // 按名字找屏外控件（rect x < -1000），隐藏锚专用：行里那颗同名开关不能当成锚 | find an off-screen widget by name (rect x < -1000), for hidden anchors only: the same-named toggle in the row must not be mistaken for the anchor
    if (!w || dep > 14) return 0; const char *nm = (const char *)*(uint64_t *)(w + 8);
    if (nm && !strcmp(nm, name) && ((int32_t *)(w + 0x70))[0] < -1000) return w;
    uint64_t ch = *(uint64_t *)(w + 0x58), n = *(uint64_t *)(w + 0x60); if (!ch || n > 256) return 0;
    for (uint64_t i = 0; i < n; i++) { uint64_t r = find_w_off(*(uint64_t *)(ch + 8 * i), name, dep + 1); if (r) return r; }
    return 0;
}
// 勾选框的状态（本机逐字节差分，点一下只变这几处）：+0xa88 是它自己的「勾上」标志（点击按它翻转）；
// +0x6e8..+0x6f4 是「常态 / 按下 / 悬停 / 禁用」四个帧号，勾上 = 4 5 6 -1、没勾 = 0 1 3 2（= 布局里 toggled* / untoggled* 那几项）；+0x610 是此刻画的帧
// 读写这几处之前核控件是勾选框那个类（dm_widget.h）：不是就当没勾、不写，并记红
// Checkbox state (byte-by-byte diff on this machine; one tap changes only these places): +0xa88 is its own "checked" flag (a click toggles it);
// +0x6e8..+0x6f4 are the four frame ids "normal / pressed / hover / disabled", checked = 4 5 6 -1, unchecked = 0 1 3 2 (= the toggled* / untoggled* entries in the layout); +0x610 is the frame drawn right now
// Before reading or writing these, check the widget is of the checkbox class (dm_widget.h): if not, treat it as unchecked, do not write, and log a failure
DM_WIDGET_RED_DEF
#define TOGGLED(w) (dm_widget_is(main_base(), (w), DM_VT_TOGGLE) && *(uint8_t *)((w) + 0xa88) != 0)
static void set_toggle(uint64_t w, int on) {
    if (!DM_WIDGET_OK(main_base(), w, DM_VT_TOGGLE, "选项页")) return;
    static const int32_t fr[2][4] = { { 0, 1, 3, 2 }, { 4, 5, 6, -1 } };
    *(uint8_t *)(w + 0xa88) = on != 0; memcpy((void *)(w + 0x6e8), fr[on != 0], sizeof fr[0]); *(int32_t *)(w + 0x610) = fr[on != 0][0];
}
// Native option bindings read the hidden toggle when handling their action.
// Prepare it at delivery time, after any native per-frame resynchronization.
void dm_custom_prepare_ui_message(unsigned long long root,const char *cat,const char *act) {
    if (!root || !cat || !act || strcmp(cat,"OptionsPanelMessage")) return;
    const char *name=!strcmp(act,"SetShowLifeOrbText")?"ShowLifeOrbText":!strcmp(act,"SetShowManaOrbText")?"ShowManaOrbText":NULL;
    if (!name || !main_base()) return;
    uint64_t panel=find_w(root,"GameplayOptionsPanel",1);
    uint64_t anchor=panel && *(uint8_t *)(panel+0x50)?find_w_off(panel,name,1):0;
    if (anchor) set_toggle(anchor,1);
}
// 触屏模式隐藏原版手柄焦点金框（FocusManager 的 FocusIndicator）：它是画在焦点控件上的九宫格金框、在顶层渲染、不受设置页滚动区裁剪，
// 焦点落在轮播 / 滑条（宽而扁）上时看起来就是一条横条 —— 用户说的「条状高亮」就是它（不是控件上的 focusIndicator 悬停图）。
// +0x50 可见位的写是登记例外（tests/native_write_scan.py）：函数里先核控件名，不是 FocusIndicator 一律不写并报红。
// Hide the original gamepad focus gold frame in touch mode (FocusManager's FocusIndicator): it is a nine-tile gold frame drawn on the
// focused widget in the top layer and is not clipped by the settings page's scroll view; on wide-and-short widgets (carousels /
// sliders) it looks like a bar -- that is the "bar highlight" the user reported (not the per-widget focusIndicator hover image).
// Writing the +0x50 visible flag is a registered exception (tests/native_write_scan.py): the function checks the widget name first and
// never writes to anything else (it reports a failure instead).
static void fi_show(uint64_t w, int on) {
    const char *nm = (const char *)*(uint64_t *)(w + 8);
    if (!nm || strcmp(nm, "FocusIndicator")) { static int lg; if (!lg) { lg = 1; LOG("【红】高亮条：要隐藏的不是 FocusManager 的 FocusIndicator（%s），不写", nm ? nm : "?"); } return; }
    if (*(uint8_t *)(w + 0x50) != (uint8_t)on) *(uint8_t *)(w + 0x50) = (uint8_t)on;
}


// 多项单选行（语言五项、镜头视角三项）：几个勾选框互斥、存一个键 i（值 = 选中第几项）。页刚出现（布局里一律没勾）先摆成该显示的值、这一帧不当玩家点了；
// 之后点没选中的那一项 = 选它，点已选中的那一项不变（摆回勾上）。改了只记待定值，点「应用」才生效
// Multi-option radio rows (language: five, camera view: three): the checkboxes are mutually exclusive and store one key i (value = which option is selected).
// When the page has just appeared (everything unchecked in the layout), first set what should show and do not treat this frame as a tap; after that,
// tapping an unselected option = select it, tapping the selected one changes nothing (it is re-checked). A change only records the pending value; Apply makes it take effect
static void radio_pump(uint64_t sp, int i, const char *const *box, const char *const *name, int n, dm_wref *seen) {
    uint64_t bx[8]; int ok = n <= 8; for (int k = 0; ok && k < n; k++) { bx[k] = find_w(sp, box[k], 0); ok &= bx[k] && *(uint8_t *)(bx[k] + 0x50); }
    if (!ok) return;
    int pd0 = atomic_load(&g_pend[i]), shown = pd0 >= 0 ? pd0 : atomic_load(&g_val[i]), to = shown;
    if (dm_wref_get(seen) != bx[0]) dm_wref_set(seen, bx[0]);
    else for (int k = 0; k < n; k++) if (k != shown && TOGGLED(bx[k])) { to = k; break; }
    if (to != shown) { atomic_store(&g_pend[i], to); LOG("自定义设置：%s → %s（点「应用」生效）", k_keys[i], name[to]); }
    for (int k = 0; k < n; k++) if (TOGGLED(bx[k]) != (k == to)) set_toggle(bx[k], k == to);
}
void dm_custom_ui_pump(unsigned long long root) {
    if (!root || !main_base()) return;
    // T41：触屏模式不画原版手柄焦点金框（宽控件上就是那条「高亮条」；手柄模式保留——它是手柄导航的焦点框） | T41: do not draw the original gamepad focus gold frame in touch mode (the "bar highlight" on wide widgets); keep it in controller mode, where it is the gamepad navigation focus frame
    { int dm_padmode_current(void); if (!dm_padmode_current()) {
        uint64_t fm = find_w(root, "FocusManager", 0), fi = fm ? find_w(fm, "FocusIndicator", 0) : 0;
        if (fi) { static int lg; if (*(uint8_t *)(fi + 0x50) && !lg) { lg = 1; LOG("高亮条：触屏模式隐藏原版手柄焦点金框（FocusManager/FocusIndicator +0x50=0）"); } fi_show(fi, 0); } } }
    static dm_wref seen[NK];   // 每行最近见到的勾选框（带界面代号：重建后取到 0 = 页刚出现） | Most recently seen checkbox of each row (with UI generation: 0 after a rebuild = page just appeared)
    uint64_t sp = dm_ui_top(root, "VideoOptionsPanel"); if (!sp || !*(uint8_t *)(sp + 0x50)) {   // 「自定义」页 = 借来的视频页（界面根的直接子，与设置页并列）。页不在屏上：没点「应用」的勾选作废（同原版不应用就离开），下次出现重摆 | The 「自定义」(Custom) page = the borrowed video page (a direct child of the UI root, alongside the settings page). Page not on screen: checkbox changes without tapping 「应用」(Apply) are discarded (same as leaving without applying in the original game), and it is laid out again the next time it appears
        int was = 0; for (int i = 0; i < NK; i++) was |= seen[i].w != 0;
        if (was) for (int i = 0; i < NK; i++) atomic_store(&g_pend[i], -1);
        memset(seen, 0, sizeof seen); return; }
    load_once(); if (atomic_exchange(&g_repaint, 0)) memset(seen, 0, sizeof seen);
    if (atomic_load(&g_lang_explicit) < 0) atomic_store(&g_val[0], !dm_lang_zh());   // 语言那一格没点过应用：生效值 = 此刻实际在跑的语言（读盘时游戏还没设语言，那时取到的不准） | Apply never tapped for the language cell: effective value = the language actually running right now (when read from disk the game has not set the language yet, so the value fetched then is not accurate)
    // 手柄模式：原版选项页的焦点只认表格里它自己的行，够不着我们的勾选框——十字键上下选行（原版手柄选中框 TouchCustomSel 框住这一行）、A 翻勾选（下面照「玩家点了」处理），其余键照旧归原版
    // Controller mode: the original options page's focus only recognizes its own rows in the table and cannot reach our checkboxes. D-pad up/down selects a row
    // (the original controller selection frame TouchCustomSel frames that row), A toggles the checkbox (handled below as "the player tapped it"), and all other
    // buttons go to the original as before
    { int dm_padmode_current(void); uint64_t dm_padmode_page_keys(uint64_t); static int sel; static dm_wref sel_sp; int pad = dm_padmode_current();
      { if (dm_ui_modal_active(root)) pad = 0; }   // 原版「设定已更改，应用 / 放弃」确认框开着：键全还给它（实测：不让的话 A 落在我们这一行上、点不到「应用」） | The original "Settings changed, Apply / Discard" confirmation box is open: hand all keys back to it (measured: otherwise A lands on our row and "Apply" cannot be reached)
      if (dm_wref_get(&sel_sp) != sp) { dm_wref_set(&sel_sp, sp); sel = 1; }
      uint64_t dm = find_w(sp, "TouchCustomSel", 0), tw = 0, cw = 0; char nm[64];
      if (pad) { uint64_t ev = dm_padmode_page_keys(DM_BTN_UP | DM_BTN_DOWN | DM_BTN_A);
          if (ev & DM_BTN_DOWN) { if (sel + 1 < K_PAGE_ROWS) ++sel; if (sel == 3) ++sel; } else if (ev & DM_BTN_UP) { if (sel > 1) --sel; if (sel == 3) --sel; }
          snprintf(nm, sizeof nm, "TouchCustom_%s", k_keys[sel]); cw = find_w(sp, nm, 0); snprintf(nm, sizeof nm, "TouchCustom_%s_Text", k_keys[sel]); tw = find_w(sp, nm, 0);
          if ((ev & DM_BTN_A) && cw && dm_wref_get(&seen[sel]) == cw) {
              if (sel == K_Q3D) { int p0 = atomic_load(&g_pend[sel]), cur = p0 >= 0 ? p0 : atomic_load(&g_val[sel]); int nx = (cur + 1) % 3;
                  atomic_store(&g_pend[sel], nx);
                  for (int q = 0; q < 3; q++) { uint64_t b = find_w(sp, q3d_box[q], 0); if (b) set_toggle(b, q == nx); }
                  LOG("自定义设置：%s → %s（手柄 A 在三项间切换；点「应用」生效）", k_keys[sel], q3d_name(nx)); }
              else if (sel == K_TOUCH_SWAP) { int p0 = atomic_load(&g_pend[sel]), sh = p0 >= 0 ? p0 : atomic_load(&g_val[sel]); atomic_store(&g_pend[sel], !sh); set_toggle(cw, !sh); { uint64_t z = find_w(sp, rev_box(sel), 0); if (z) set_toggle(z, sh); }
                  LOG("自定义设置：%s → %d（手柄 A 在两项间切换；点「应用」生效）",k_keys[sel],!sh); }
              else if (sel == K_CAM) { int p0 = atomic_load(&g_pend[K_CAM]), sh = p0 >= 0 ? p0 : atomic_load(&g_val[K_CAM]), to = (sh + 1) % CAM_N; atomic_store(&g_pend[K_CAM], to);   // 镜头视角三项：A 依次轮换 | camera view: A cycles through the three
                  for (int k = 0; k < CAM_N; k++) { uint64_t z = find_w(sp, k_cam_box[k], 0); if (z) set_toggle(z, k == to); }
                  LOG("自定义设置：镜头视角 → %s（手柄 A 在三项间轮换；点「应用」生效）", k_cam_name[to]); }
              else if (sel == 0) { int p0 = atomic_load(&g_pend[0]), sh = p0 >= 0 ? p0 : atomic_load(&g_val[0]); atomic_store(&g_pend[0], !sh); set_toggle(cw, !sh); { uint64_t z = find_w(sp, "TouchCustom_lang_zh", 0); if (z) set_toggle(z, sh); }   // 语言行是单选的两项：两个框当场摆好（只改待定值的话，下面读框状态会把没跟上的框当成玩家点的） | The language row is a single choice between two items: both boxes are set on the spot (if only the pending value were changed, the code below that reads the box states would take the box that has not caught up as a player tap)
                  LOG("自定义设置：语言 → %s（手柄 A 在两项间切换；点「应用」生效）", !sh ? "English" : "中文"); }
              else set_toggle(cw, !TOGGLED(cw)); } }
      if (dm) { int32_t *r = (int32_t *)(dm + 0x70), want[4] = { 99999, 0, r[2], r[3] };   /* 不用时只把 x 挪出屏：它在滚动区里，y 放远了会撑大可滚范围 | when unused only x is moved off screen: it is inside the scroll area, and a far-away y would enlarge the scrollable range */
          if (dm_padmode_current() && tw && cw) { int32_t *t = (int32_t *)(tw + 0x70), *c = (int32_t *)(cw + 0x70); int top = t[1] < c[1] ? t[1] : c[1], bot = t[1] + t[3] > c[1] + c[3] ? t[1] + t[3] : c[1] + c[3];   // 框住整行：文字与勾选框两者的上下沿取并，再各放 10 单位（原先只按文字框的高度算，文字框比字矮，框成了横穿文字中线的一条） | Frame the whole row: take the union of the top and bottom edges of the text and the checkbox, then pad 10 units on each side (previously computed from the text box height only; the text box is shorter than the glyphs, so the frame became a strip cutting across the middle of the text)
              want[0] = t[0] - 30; want[1] = top - 10; want[2] = c[0] + c[2] + 30 - want[0]; want[3] = bot - top + 20;
              { static int lg; if (lg++ < 2) LOG("自定义设置：手柄选中框 文字框 %d,%d %dx%d 勾选框 %d,%d %dx%d → 框 %d,%d %dx%d", t[0], t[1], t[2], t[3], c[0], c[1], c[2], c[3], want[0], want[1], want[2], want[3]); } }
          if (memcmp(r, want, sizeof want)) { memcpy(r, want, sizeof want); ((void (*)(uint64_t))*(uint64_t *)(*(uint64_t *)dm + 0x10))(dm); } }
      // 这页可以滚（各行在原版滚动区里，touch_layout.py _custom_page）：手柄选到列表框外的行时，让原版滚动区自己滚到位——发滚轮，每 4 帧一格，直到这一行整行在框内。
      // 已滚的量 = 内层视图（行容器的父控件）矩形 y 的相反数；框高 = 滚动区（再上一级）的高。只读这两个数，不写原版控件
      // This page can scroll (the rows are in the original scroll area, touch_layout.py _custom_page): when the controller selects a row outside the list box, let the original scroll area scroll into place by itself -- send wheel events, one notch every 4 frames, until the whole row is inside the box.
      // Amount scrolled = the negative of the rect y of the inner view (the parent widget of the row container); box height = the height of the scroll area (one level further up). Only these two numbers are read; the original game's widgets are not written
      if (pad && tw && cw) { uint64_t rw = find_w(sp, "TouchCustomRows", 0), view = rw ? *(uint64_t *)(rw + 0x30) : 0, sv = view ? *(uint64_t *)(view + 0x30) : 0; static uint64_t lf; uint64_t dm_guest_frame(void); uint64_t f = dm_guest_frame();
          if (sv && f - lf >= 4) { int32_t *t = (int32_t *)(tw + 0x70), *c = (int32_t *)(cw + 0x70); int top = (t[1] < c[1] ? t[1] : c[1]) - 10, bot = (t[1] + t[3] > c[1] + c[3] ? t[1] + t[3] : c[1] + c[3]) + 10;
              int off = -*(int32_t *)(view + 0x74), vh = *(int32_t *)(sv + 0x7c); if (vh <= 0) vh = 1260; void dm_ui_wheel_on(uint64_t, int); if (sel == 0) top = 0;   /* 选回第一行：一直滚到最顶，把标题行露出来 | selection back on the first row: scroll all the way to the top so that the header row is revealed */
              if (top < off) { lf = f; dm_ui_wheel_on(sv, 1); } else if (bot > off + vh) { lf = f; dm_ui_wheel_on(sv, -1); } } } }
    radio_pump(sp, K_CAM, k_cam_box, k_cam_name, CAM_N, &seen[K_CAM]);
    for (int i = 0; i < NK; i++) { if (i == K_CAM) continue; char nm[48]; snprintf(nm, sizeof nm, "TouchCustom_%s", k_keys[i]);
        uint64_t w = find_w(sp, nm, 0); if (!w || !*(uint8_t *)(w + 0x50)) continue;
        int pd0 = atomic_load(&g_pend[i]), want = pd0 >= 0 ? pd0 : atomic_load(&g_val[i]);
        if (i == K_Q3D) {   // 性能模式三选一：三框互斥 | performance mode, three-way radio: the three boxes are mutually exclusive
            uint64_t b[3]; int haveall = 1;
            for (int q = 0; q < 3; q++) { b[q] = find_w(sp, q3d_box[q], 0); if (!b[q] || !*(uint8_t *)(b[q] + 0x50)) haveall = 0; }
            if (!haveall) continue;   // 布局还是旧的两项版：这一行不处理（等新布局） | old two-option layout: skip this row until the new layout is in
            if (dm_wref_get(&seen[i]) != b[1]) { dm_wref_set(&seen[i], b[1]); for (int q = 0; q < 3; q++) set_toggle(b[q], q == want); continue; }   // 页刚出现：先摆好 | page just appeared: lay it out first
            int to = want;
            for (int q = 0; q < 3; q++) if (TOGGLED(b[q]) && q != want) { to = q; break; }   // 点了另一个选项 | another option was tapped
            if (to != want) { atomic_store(&g_pend[i], to); LOG("自定义设置：%s → %s（点「应用」生效）", k_keys[i], q3d_name(to)); }
            for (int q = 0; q < 3; q++) if (TOGGLED(b[q]) != (q == to)) set_toggle(b[q], q == to);
            continue;
        }
        uint64_t z = rev_box(i) ? find_w(sp, rev_box(i), 0) : 0;   // 单选行（语言：并排的「中文」「English」；性能模式：上下的「流畅优先」「画质优先」）：存的仍是一个键，反面那一项是它的反面 | Radio row (language: side-by-side "中文" / "English"; performance mode: stacked "Smoothness first" / "Quality first"): still stores a single key, and the other option is its inverse
        if (dm_wref_get(&seen[i]) != w) { dm_wref_set(&seen[i], w); set_toggle(w, want); if (z) set_toggle(z, !want); continue; }   // 页刚出现（布局里一律是没勾）：先摆成该显示的值，这一帧不当玩家点了 | The page has just appeared (in the layout everything is unchecked): first set it to the value it should show, and do not treat this frame as a player tap
        int on = TOGGLED(w), pd = atomic_load(&g_pend[i]), shown = pd >= 0 ? pd : atomic_load(&g_val[i]);
        if (z) {   // 点没选中的那一项 = 选它；点已选中的那一项不变（摆回勾上） | Tapping the item that is not selected = select it; tapping the one already selected changes nothing (it is put back to checked)
            int zon = TOGGLED(z), to = on != shown ? (on ? 1 : shown) : zon != !shown ? (zon ? 0 : shown) : shown;
            if (to != shown) { atomic_store(&g_pend[i], to); if (i == 0) LOG("自定义设置：语言 → %s（点「应用」生效）", to ? "English" : "中文"); else LOG("自定义设置：%s → %d（点「应用」生效）",k_keys[i],to); }
            if (TOGGLED(w) != to) set_toggle(w, to); if (TOGGLED(z) != !to) set_toggle(z, !to); continue; }
        if (on != shown) { atomic_store(&g_pend[i], on); LOG("自定义设置：%s 勾选框 → %s（点「应用」生效）", k_keys[i], on ? "勾上" : "取消"); } }
    // 语言那一行下面的「点应用后把 App 关掉重开」提示（TouchCustomLangHint，布局里先放在屏外）：勾选框此刻显示的值与正在跑的语言不一致时挪到标签正下方（横坐标取标签的）
    // The "tap Apply, then close and reopen the app" hint under the language row (TouchCustomLangHint, placed off screen in the layout): moved right under the label (taking the label's x) when the value the checkboxes currently show differs from the language now running
    { uint64_t lw = find_w(sp, "TouchCustom_lang_en_Text", 0); int pd = atomic_load(&g_pend[0]), shown = pd >= 0 ? pd : atomic_load(&g_val[0]), diff = shown != !dm_lang_zh();
      for (int k = 0; k < 2 && lw; k++) { uint64_t hw = find_w(sp, k ? "TouchCustomLangHintEn" : "TouchCustomLangHintZh", 0); if (!hw) continue;   // 提示用「选中的那种语言」写：选了中文显示中文那句，选了 English 显示英文那句 | The hint is written in "the language that was chosen": choosing 中文 shows the Chinese sentence, choosing English shows the English one
          int x = diff && shown == k ? ((int32_t *)(lw + 0x70))[0] : 99999; int32_t *r = (int32_t *)(hw + 0x70);
          if (r[0] != x) { r[0] = x; ((void (*)(uint64_t))*(uint64_t *)(*(uint64_t *)hw + 0x10))(hw); } } }
    // 「应用 / 放弃」亮不亮（自定义页改了勾选，「应用」仍是灰的）：原版各选项子页的每帧更新 main+0xae5f28 把自己 +0x1a0「有未应用的改动」
    // 交给 main+0xaed9e4(设置页, 亮否)，按钮 +0x57a 是灰的标志；原版的勾选框一改就由面板置 +0x1a0，我们的勾选框原版面板不认识，这一位一直是 0。
    // 按待定值与生效值比（改回原样 = 没改动 = 灭）写这一位，走原版自己的亮灭与「有改动离开要确认」。只在面板虚表里确有 0xae5f28 时写（确认 +0x1a0 是这个意思）
    // Whether 「应用 / 放弃」(Apply / Discard) light up (a checkbox on the custom page was changed, yet 「应用」(Apply) is still gray): the per-frame update main+0xae5f28 of each original-game option subpage passes its own +0x1a0 'has unapplied changes'
    // to main+0xaed9e4(settings page, lit or not), and button +0x57a is the gray flag; an original-game checkbox has +0x1a0 set by the panel as soon as it changes, but the original panel does not know our checkboxes, so this bit stays 0.
    // Write this bit by comparing pending values with effective values (changed back to the original = no change = off), using the original game's own lit / unlit behavior and its 'confirm when leaving with changes'. Written only when the panel vtable really contains 0xae5f28 (confirming that +0x1a0 has this meaning)
    static uint64_t vt_ok_for; static int vt_ok; if (vt_ok_for != *(uint64_t *)sp) { vt_ok_for = *(uint64_t *)sp; vt_ok = 0;
        for (int k = 0; k < 96 && !vt_ok; k++) vt_ok = *(uint64_t *)(vt_ok_for + 8 * k) == (uint64_t)main_base() + 0xae5f28;
        if (!vt_ok) LOG("【红】自定义设置：借来的视频页虚表里没有 main+0xae5f28，不写「有改动」位（原版版本变了？）"); }
    if (vt_ok) { int dirty = 0; for (int i = 0; i < NK; i++) { int pd = atomic_load(&g_pend[i]); dirty |= pd >= 0 && pd != atomic_load(&g_val[i]); }
        if (*(uint8_t *)(sp + 0x1a0) != dirty) { *(uint8_t *)(sp + 0x1a0) = (uint8_t)dirty; LOG("自定义设置：有未应用的改动 → %d（「应用 / 放弃」%s）", dirty, dirty ? "点亮" : "变灰"); } }
}

// ---- 游戏设置页「显示帧率」勾选框 + 生命 / 法力球文本常开（T18/T19/T42）：设置 → 游戏 → 用户界面 分区。
// T42（方案 C）：这一分区只有两行 —— 「显示技能上的弹药数量」（原版行，内容不动）→ 「显示帧率」（复用原版法力球文本行）。
// 67.1 帧率行看不见：面板按原版选项认行（行里的原版子控件名）；T36 把名字都换了 → 整行隐藏。68.1（方案 B）在行里塞了
// 4 个子控件（文字框 + 2 个隐藏锚 + 可见开关），但原版 TableRowWidget 按表格列数（2 列）排版，子控件数 > 列数时整行跳过排版、
// 行高算成 0 → 真机上仍然错位 / 看不见。方案 C：帧率行只留 2 个子控件 —— 原版文字框（只换文字）+ 原版 ShowManaOrbText 开关
// 本身（名字 / rect 原样，onClickMessage 换成空操作 PanelManager:ClosePanel:TouchCustom_show_fps），宿主把这颗开关当帧率勾选框。
// 两颗原版球文本开关留成屏外隐藏锚（名字 / 消息原样，rect x=-99999），放在面板根 children 最前面：面板按名字找控件时先找到锚
// → 选项绑在锚上，行里那颗同名开关不会被面板重置。两项球文本由这里读两个锚的勾选状态（+0xa88）并置开：为关就发
// OptionsPanelMessage:SetShowLifeOrbText / SetShowManaOrbText（每项最多 2 次、发后隔 10 帧读回），读回变开即成功；2 次仍读回关
// 则打红字（说明消息可能是 toggle 语义或面板读不回状态——那时只能保证游戏默认开，见 dm_custom.h 的说明）。
// 点一下帧率勾选框立即生效并存盘（键 show_fps，重启保留）；设置页重建 / 重开时按生效值重摆。
// Two rows only in Settings > Game > User Interface (T42, scheme C): Show Skill Ammo Feedback (original row, contents untouched) ->
// Show Frame Rate (the original mana-orb-text row reused). The frame-rate row was invisible in 67.1 because the options panel
// recognises rows by the original option fingerprint (original child widget names); T36 renamed them, so the panel hid the whole row.
// Scheme B (68.1) put four children in the row (text box + two hidden anchors + a visible toggle), but the original TableRowWidget
// lays children out by the table's column count (two columns): when the child count exceeds the column count it skips the whole row
// layout and computes a zero row height, so on the device the row is still misplaced / invisible. Scheme C keeps exactly two
// children -- the original text box (text changed only) + the original ShowManaOrbText toggle itself (name / rect untouched,
// onClickMessage replaced with the no-op PanelManager:ClosePanel:TouchCustom_show_fps) -- and the host uses that toggle as the
// frame-rate checkbox. Both original orb-text toggles stay as off-screen hidden anchors (name / message untouched, rect x=-99999)
// at the very front of the panel root's children: the panel's name lookup finds the anchor first, so the option binds to the anchor
// and the same-named toggle inside the row is never reset by the panel. Both orb texts are forced on here by reading the anchors'
// checked state (+0xa88): when off we send OptionsPanelMessage:SetShowLifeOrbText / SetShowManaOrbText (at most 2 tries per item,
// re-read 10 frames after each); if it is still off after 2 tries a red line is logged (that means the message is toggle-semantics
// or the panel does not reflect the state -- in that case only the game default "on" is guaranteed, see dm_custom.h). Tapping the
// frame-rate checkbox takes effect and saves immediately (key show_fps, kept across restarts); it is laid out again from the
// effective value when the page is rebuilt / reopened.
void dm_show_fps_page_pump(unsigned long long root) {
    if (!root || !main_base()) return;
    static dm_wref seen; static int was_open, logged_missing, flips, logged_order; static uint64_t flip_fr;
    typedef struct { const char *name, *msg, *label; int tries, on, bad; uint64_t wait; } orb_fix;
    static orb_fix fx[2] = {
        { "ShowLifeOrbText", "OptionsPanelMessage:SetShowLifeOrbText", "生命球文本", 0, 0, 0, 0 },
        { "ShowManaOrbText", "OptionsPanelMessage:SetShowManaOrbText", "法力球文本", 0, 0, 0, 0 } };
    uint64_t sp = find_w(root, "GameplayOptionsPanel", 1);
    int open = sp && *(uint8_t *)(sp + 0x50);
    if (!open) { was_open = 0; return; }
    if (!was_open) {   // 页刚打开：重新同步一次 | page just opened: resync once
        was_open = 1; dm_wref_set(&seen, 0); logged_missing = 0; flips = 0; logged_order = 0;
        for (int i = 0; i < 2; i++) { fx[i].tries = 0; fx[i].on = 0; fx[i].bad = 0; fx[i].wait = 0; } }
    load_once();
    // 新布局：帧率行沿用原版的 Row ShowManaOrbText 行名（面板认识 → 占行高 / 重排）；65.1 旧包回退到旧行名，行为不变
    // new layout: the frame-rate row keeps the original Row ShowManaOrbText name (known to the panel -> row height / re-layout); the old
    // 65.1 package falls back to the old row name, same behaviour
    uint64_t row = find_w(sp, "Row ShowManaOrbText", 1);
    if (!row) row = find_w(sp, "TouchCustom_RowShowFps", 1);
    if (!row) { if (!logged_missing) { logged_missing = 1; LOG("显示帧率：设置页里没找到帧率行（Row ShowManaOrbText / TouchCustom_RowShowFps），布局补丁没进包？"); } return; }
    if (!*(uint8_t *)(row + 0x50)) { static int hlog; if (!hlog) { hlog = 1; LOG("【红】显示帧率：帧率行被原版面板隐藏（+0x50=0）——行里没有面板认识的原版子控件？"); } }
    // 方案 C：行里那颗原版 ShowManaOrbText 开关本身就是帧率勾选框（不再另插 TouchCustom_show_fps）
    // scheme C: the original ShowManaOrbText toggle in the row is the frame-rate checkbox itself (no extra TouchCustom_show_fps)
    uint64_t w = find_w(row, "ShowManaOrbText", 1);
    if (!w) { if (!logged_missing) { logged_missing = 1; LOG("显示帧率：帧率行里没找到 ShowManaOrbText（布局补丁没生效？）"); } return; }
    // 用户界面分区最终可见顺序（T42）：两行都在才打一次，真机核对用 | final visible order of the User Interface section (T42): logged once when both rows exist, for real-device checks
    if (!logged_order) {
        uint64_t ar = find_w(sp, "Row SkillAmmoFeedback", 1), fr = find_w(sp, "Row ShowManaOrbText", 1);
        if (ar && fr) { logged_order = 1;
            LOG("设置页用户界面分区可见开关顺序：显示技能上的弹药数量 → 显示帧率（共两行；生命球 / 法力球文本已强制常开，它们的开关不再显示）"); }
    }
    // 生命 / 法力球文本常开：读隐藏锚的勾选状态，为关就发对应消息（每项最多 2 次、隔 10 帧读回）
    // Force both orb texts on: read the hidden anchors' checked state and, when off, send the matching message (at most 2 tries per item,
    // re-read 10 frames after each)
    { uint64_t fr = dm_guest_frame();
      for (int i = 0; i < 2; i++) {
        uint64_t a = find_w_off(sp, fx[i].name, 1);   // 锚在面板根最前面，按屏外 rect 认，别拿到行里那颗同名开关 | anchors are at the panel root; match the off-screen rect so the same-named toggle in the row is never taken
        if (!a) { if (!fx[i].bad) { fx[i].bad = 1; LOG("【红】%s：面板根没有屏外隐藏锚 %s（布局补丁没进包？）", fx[i].label, fx[i].name); } continue; }
        if (TOGGLED(a)) { if (!fx[i].on) { fx[i].on = 1; LOG("%s：隐藏锚点读回=开（本版强制常开）", fx[i].label); } continue; }
        if (fx[i].on) continue;   // 本页此前读回过开：不再动（等游戏重排再看） | this page reported it on before: leave it alone (look again after the next rebuild)
        if (fx[i].tries < 2 && fr >= fx[i].wait) {
            fx[i].tries++; fx[i].wait = fr + 10;
            set_toggle(a,1);
            dm_ui_send_queue(fx[i].msg);
            LOG("%s：隐藏锚点置为开 → 排队发 %s（第 %d/2 次，10 帧后读回）", fx[i].label, fx[i].msg, fx[i].tries);
        } else if (fx[i].tries >= 2 && fr >= fx[i].wait && !fx[i].bad) {
            fx[i].bad = 1;
            LOG("【红】%s：发了 2 次仍读回=关（消息可能是 toggle 语义或面板读不回状态）；本版只能保证游戏默认开，请在设置页手动确认一次", fx[i].label);
        } } }
    int val = atomic_load(&g_val[K_SHOW_FPS]);
    if (dm_wref_get(&seen) != w) {   // 页刚出现 / 控件重建：先摆成生效值，这一帧不当玩家点了 | page just appeared / widget rebuilt: lay it out from the effective value, do not treat this frame as a player tap
        dm_wref_set(&seen, w); set_toggle(w, val);
        LOG("显示帧率：设置页勾选框同步为%s", val ? "开" : "关");
        return;
    }
    int on = TOGGLED(w);
    if (on != val) {
        uint64_t fr = dm_guest_frame();   // 防抖：原版面板若每帧把我们这格重置（不认识的控件），别变成「每帧写盘」的振荡 | debounce: if the original panel resets our cell every frame (it does not know the widget), avoid a per-frame disk-write oscillation
        if (fr - flip_fr > 24) flips = 0;
        if (++flips > 6) { if (!logged_missing) { logged_missing = 1; LOG("【红】显示帧率：勾选框被面板每帧重置，停止同步（不写盘；等设置页重开）"); } return; }
        flip_fr = fr;
        LOG("显示帧率：%s（游戏设置页勾选框，立即生效并存盘）", on ? "开" : "关");
        dm_custom_set_live("show_fps", on);
    }
}
int dm_show_fps_on(void) { return dm_custom_get("show_fps"); }
