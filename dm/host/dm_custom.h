// 「自定义」设置：我们自己加的功能的启停开关，统一在这里读写。各模块只调 dm_custom_get，不各自读文件 / 环境变量当设置。
// 「自定义」(Custom) settings: on/off switches for the features we added ourselves, all read and written here. Each module only calls dm_custom_get and does not read files / environment variables as settings on its own.
#pragma once

// 已生效的值 0 / 1：只在玩家点选项页「应用」后变，「放弃」不变，「默认」= 各键缺省（其余开）。键不认识返回 1。
// 任意线程可调：第一次调用读一次文件，之后只是原子量读 + 几项字符串比较，无锁。iOS 上启动期（存档挂上之前）就能读。
// 键：auto_mana 自动喝药水（红蓝两种，键名沿用）/ drop_light 高价值掉落光柱 / map_mark 小地图标注出入口、小站与任务终点 / item_notes 物品注释与旧译名 / icloud iCloud 同步存档（cloud_ios.m，启动时读）/ pot_pick 药水自动拾取（dm_potpick.c）/ drop_haptic 高价值掉落震动（dm_drophaptic.c） / gem_pick 符文宝石自动拾取进高级仓库（百宝囊）/ store_btn 背包「存放」按钮（dm_pouch.c）/ q3d 性能模式（0 流畅优先 / 1 画质优先）/ show_fps 局内左下角显示帧率（游戏设置页「用户界面」分区勾选框，缺省关）
// touch_swap_above：0 = 传送门左侧（默认），1 = 技能区上方；点「应用」后生效。
// Effective value 0 / 1: changes only after the player taps Apply on the options page; Discard leaves it unchanged; Default = each key's default (the rest on). Returns 1 for an unknown key.
// Callable from any thread: the first call reads the file once, after that it is an atomic read plus a few string comparisons, lock-free. On iOS it can be read during startup (before the save is mounted).
// Keys: auto_mana auto-drink potions (red and blue; key name kept) / drop_light light beam on high-value drops / map_mark minimap markers for entrances and exits, waypoints and quest goals / item_notes item notes and old translated names / icloud iCloud save sync (cloud_ios.m, read at startup) / pot_pick potion auto-pickup (dm_potpick.c) / drop_haptic haptics on high-value drops (dm_drophaptic.c) / gem_pick auto-pickup of runes and gems into the advanced stash (pouch) / store_btn the inventory Deposit button (dm_pouch.c) / q3d performance mode (0 smoothness first / 1 quality first) / show_fps show the in-game FPS at the bottom-left (checkbox in the game's Settings > Game > User Interface section, off by default)
// touch_swap_above: 0 = left of portal (default), 1 = above the skill fan; applies after Apply.
int dm_custom_get(const char *key);
int dm_custom_lang_explicit(void);   // 语言开关（键 lang_en）在设置文件里写着的值：-1 没写过（不动原版设置）/ 0 简中 / 1 英文；hle_fs.c 挂存档时读 | Value of the language switch (key lang_en) as written in the settings file: -1 never written (original settings left alone) / 0 Simplified Chinese / 1 English; read by hle_fs.c at save mount
// q3d 性能模式（缺省 0 = 流畅优先：掉帧自动降 3D 比例；1 = 画质优先：不自动降，过热仍降档；2 = 画质优先（强制）：固定 60 帧 / 画质，忽略过热降档），读盘 / 应用时调 hle_oe.c 的 dm_3d_quality_mode_set 生效
// q3d performance mode (default 0 = smoothness first: automatically lowers the 3D scale on frame drops; 1 = quality first: no automatic lowering, thermal may still lower it; 2 = quality first (forced): fixed 60 fps / quality, ignores thermal downgrade); applied on load / apply by calling hle_oe.c's dm_3d_quality_mode_set
int dm_fps_setting(void);   // 帧率上限：温控决定（正常 60 / 温热 30），但 q3d=2（强制画质）恒 60；nvn_mtl.m mtl_fps_target 读 | frame-rate cap: thermal (60 normal / 30 warm), except q3d=2 (forced quality) which is always 60; read by mtl_fps_target in nvn_mtl.m
void dm_custom_set_live(const char *key, int val);   // 运行时更改并存盘（双指缩放等手势调用） | Change at runtime and save to disk (called by gestures like pinch-to-zoom)
int dm_show_fps_on(void);   // 游戏设置页「显示帧率」是否勾上（键 show_fps；覆盖层读） | whether the settings page's "Show Frame Rate" is on (key show_fps; read by the overlay)
void dm_show_fps_page_pump(unsigned long long root);   // 游戏主线程每帧：游戏设置页「用户界面」分区的显示帧率勾选框同步 / 点击处理 | Game main thread, every frame: sync / handle taps of the show-frame-rate checkbox in the game's settings page

void dm_custom_init(const char *dir);   // 设置文件所在目录（iOS 外壳启动时给 Documents）；不调则用存档提交目录的上一级（Mac 宿主） | Directory holding the settings file (the iOS shell passes Documents at startup); if not called, the parent of the save commit directory is used (Mac host)

// 以下只给自定义页的界面接线用
// The following are only for the UI wiring of the custom page
void dm_custom_on_uimsg(const char *cat, const char *act, const char *arg);   // 选项页应用 / 默认 / 放弃（dm_cursor.c 的界面消息回调转来） | Options page apply / default / discard (forwarded from the UI message callback in dm_cursor.c)
void dm_custom_ui_pump(unsigned long long root);                               // 游戏主线程每帧：选项页开着时读各行勾选框（控件名 TouchCustom_<键>） | Game main thread, every frame: while the options page is open, read the checkbox of each row (widget name TouchCustom_<key>)
