// 暂停菜单「分享诊断日志」的 iOS 侧。
// 按钮是游戏自己的控件（布局 tools/touch_layout.py 的 _pause_no_help：借原版「消息记录」按钮，换字、改发 PanelManager:ClosePanel:TouchShareLog），
// 宿主收到这条空操作消息后调这里登记的回调（dm/host/diag.c）。这里做两件：
//   ① 打一个文件：头部（构建号、机型、系统版本、温控 / 电量 / 内存 / 存储、本局与上一局的关键性能读数）+ 本局 boot.txt + 上一局 boot.previous.txt（main.m 每次启动前改名留下）；
//   ② 弹系统分享面板（UIActivityViewController），玩家自己选发到哪；iPad / Mac 上分享面板是弹出框，必须给锚点，否则一弹就崩。
// 隐私：只放这两份日志（游戏与宿主自己写的）和上面那几项机器读数；不取设备名、不取任何账号 / 位置 / 通讯录类信息。
// 测试开关 DM_TEST_SHARELOG_CLOSE=<秒>：分享面板照常弹出，过这么多秒自动收起；导出的文件另拷一份到 Documents/sharelog_test.txt 供回归核内容。
// iOS side of the pause menu's 「分享诊断日志」(Share Diagnostics).
// The button is the game's own widget (layout: _pause_no_help in tools/touch_layout.py: borrows the original game's 「消息记录」(Message Log) button, changes the text, and makes it send PanelManager:ClosePanel:TouchShareLog instead),
// and on receiving this no-op message the host calls the callback registered here (dm/host/diag.c). Two things are done here:
//   ① Build one file: header (build number, device model, OS version, thermal / battery / memory / storage, key performance readings of this run and the previous run) + this run's boot.txt + the previous run's boot.previous.txt (left by main.m's rename before each launch);
//   ② Present the system share sheet (UIActivityViewController), and the player chooses where to send it; on iPad / Mac the share sheet is a popover and must be given an anchor, otherwise it crashes as soon as it is presented.
// Privacy: export redacts home paths, application IDs, emails, container/device identifiers and save character names. Only these two logs (written by the game and the host themselves) and the machine readings listed above are included; no device name, no account / location / contacts information of any kind.
// Test switch DM_TEST_SHARELOG_CLOSE=<seconds>: the share sheet is presented as usual and dismissed automatically after that many seconds; the exported file is also copied to Documents/sharelog_test.txt so regression can check its content.
#import <UIKit/UIKit.h>
#include <stdatomic.h>
#include <sys/sysctl.h>
#include "../../host/dm_i18n.h"
#include "../../host/dm_custom.h"
#import "dm_diagnostic_scrub.h"
#import "dm_diagnostic_read.h"
#include "../../gfx/nvn/nvn_mtl.h"
float dm_3d_scale_cur(void);
void plog(const char *fmt, ...);
void dm_sharelog_init(void (*cb)(void));   // dm/host/diag.c

// 日志里最后一条含 key 的行（不含行尾换行）；没有返回 nil
// The last line in the log containing key (without the trailing newline); returns nil if none
static NSString *last_line(NSData *d, const char *key) {
    if (!d.length) return nil;
    NSData *k = [NSData dataWithBytes:key length:strlen(key)];
    NSRange r = [d rangeOfData:k options:NSDataSearchBackwards range:NSMakeRange(0, d.length)]; if (r.location == NSNotFound) return nil;
    const char *b = d.bytes; NSUInteger s = r.location, e = r.location;
    while (s > 0 && b[s - 1] != '\n') s--;
    while (e < d.length && b[e] != '\n') e++;
    return [[NSString alloc] initWithBytes:b + s length:e - s encoding:NSUTF8StringEncoding] ?: @"（这一行不是合法 UTF-8）";
}
static void perf_block(NSMutableString *h, NSString *title, NSData *d) {
    [h appendFormat:@"\n—— %@ ——\n", title];
    if (!d) { [h appendString:@"（没有这份日志）\n"]; return; }
    static const char *keys[] = { "boot: 版本", "[诊断]", "[perf]", "[nvn] 上屏", "温控档位", "紫闪探针 至出图", "MetalFX spatial support", "MetalFX spatial ready", "MetalFX initialization unavailable", "图形设置：", "物品标签实验：" };
    for (size_t i = 0; i < sizeof keys / sizeof *keys; i++) { NSString *l = last_line(d, keys[i]); [h appendFormat:@"%@\n", l ?: [NSString stringWithFormat:@"（没有「%@」行）", @(keys[i])]]; }
}
// 日志里的路径会带系统用户主目录（Mac 上是 /Users/<用户名>，iPhone 上是 /var/mobile）：导出时一律换成 ~，不把用户名带出去
// Paths in the log carry the system user's home directory (/Users/<username> on Mac, /var/mobile on iPhone): on export always replace it with ~ so the username does not leak out
// ---- 本地改动：最小 ZIP 写入器（存储法 / method 0）----
// iOS 上没有 zip 命令（Mac 那份用的是 /usr/bin/ditto），所以这里自己写：不压缩，只要 CRC32 与三段头部，
// 出错余地最小。这份代码先在 Windows 上用 mingw 编译、用 Python 的 zipfile 校验过（CRC 全部正确、二进制逐字节一致）。
// Local change: a minimal stored-method ZIP writer. iOS has no zip command (the Mac build shells out to
// /usr/bin/ditto), so this writes one: no compression, only CRC32 and the three header kinds, the smallest possible
// surface for mistakes. It was compiled with mingw on Windows and validated with Python's zipfile first (every CRC
// correct, binary content byte-identical).
#include <stdio.h>
#include <stdint.h>
#include <string.h>
static uint32_t zs_crc_tab[256];
static uint32_t zs_crc32(const void *p, size_t n) {
    if (!zs_crc_tab[1]) for (uint32_t i = 0; i < 256; i++) { uint32_t c = i; for (int k = 0; k < 8; k++) c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1); zs_crc_tab[i] = c; }
    const uint8_t *b = (const uint8_t *)p; uint32_t c = 0xFFFFFFFFu;
    for (size_t i = 0; i < n; i++) c = zs_crc_tab[(c ^ b[i]) & 0xFF] ^ (c >> 8);
    return c ^ 0xFFFFFFFFu;
}
#define ZS_MAXENT 500
typedef struct { FILE *f; int n; struct { char name[600]; uint32_t crc, size, off; } e[ZS_MAXENT]; } zs_t;
static void zs_w16(FILE *f, unsigned v) { fputc(v & 0xFF, f); fputc((v >> 8) & 0xFF, f); }
static void zs_w32(FILE *f, uint32_t v) { zs_w16(f, v & 0xFFFF); zs_w16(f, (v >> 16) & 0xFFFF); }
static int zs_open(zs_t *z, const char *path) { memset(z, 0, sizeof *z); z->f = fopen(path, "wb"); return z->f != NULL; }
static int zs_add(zs_t *z, const char *name, const void *data, size_t n) {
    if (!z->f || z->n >= ZS_MAXENT || strlen(name) >= sizeof z->e[0].name) return 0;
    snprintf(z->e[z->n].name, sizeof z->e[0].name, "%s", name);
    z->e[z->n].crc = zs_crc32(data, n); z->e[z->n].size = (uint32_t)n; z->e[z->n].off = (uint32_t)ftell(z->f);
    zs_w32(z->f, 0x04034b50u); zs_w16(z->f, 20); zs_w16(z->f, 0); zs_w16(z->f, 0);   // method 0 = 存储 | stored
    zs_w16(z->f, 0); zs_w16(z->f, 0x5A21);                                          // 时间/日期固定 2025-01-01，合法即可 | fixed valid DOS date
    zs_w32(z->f, z->e[z->n].crc); zs_w32(z->f, z->e[z->n].size); zs_w32(z->f, z->e[z->n].size);
    zs_w16(z->f, (unsigned)strlen(name)); zs_w16(z->f, 0); fwrite(name, 1, strlen(name), z->f);
    fwrite(data, 1, n, z->f); z->n++; return 1;
}
static int zs_close(zs_t *z) {
    if (!z->f) return 0;
    uint32_t cd = (uint32_t)ftell(z->f);
    for (int i = 0; i < z->n; i++) {
        zs_w32(z->f, 0x02014b50u); zs_w16(z->f, 20); zs_w16(z->f, 20); zs_w16(z->f, 0); zs_w16(z->f, 0);
        zs_w16(z->f, 0); zs_w16(z->f, 0x5A21); zs_w32(z->f, z->e[i].crc); zs_w32(z->f, z->e[i].size); zs_w32(z->f, z->e[i].size);
        zs_w16(z->f, (unsigned)strlen(z->e[i].name)); zs_w16(z->f, 0); zs_w16(z->f, 0); zs_w16(z->f, 0); zs_w16(z->f, 0);
        zs_w32(z->f, 0); zs_w32(z->f, z->e[i].off); fwrite(z->e[i].name, 1, strlen(z->e[i].name), z->f); }
    uint32_t cdsize = (uint32_t)ftell(z->f) - cd;
    zs_w32(z->f, 0x06054b50u); zs_w16(z->f, 0); zs_w16(z->f, 0); zs_w16(z->f, (unsigned)z->n); zs_w16(z->f, (unsigned)z->n);
    zs_w32(z->f, cdsize); zs_w32(z->f, cd); zs_w16(z->f, 0);
    int ok = fclose(z->f) == 0; z->f = NULL; return ok;
}
static NSDate *g_t0;   // 本次启动时刻（dm_sharelog_boot） | Time of this launch (dm_sharelog_boot)
static NSArray<NSURL *> *build_file(void) {   // 本地改动：返回「日志 + 存档 zip」两个文件（原来只有一个 txt） | local change: returns both the log and the save zip (one txt before)
    NSFileManager *fm = NSFileManager.defaultManager;
    NSString *doc = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES)[0];
    const NSUInteger log_limit = 2 * 1024 * 1024;
    unsigned long long cur_omitted = 0, prev_omitted = 0;
    NSData *cur = dm_diagnostic_read_tail([doc stringByAppendingPathComponent:@"boot.txt"],log_limit,&cur_omitted);
    NSData *prev = dm_diagnostic_read_tail([doc stringByAppendingPathComponent:@"boot.previous.txt"],log_limit,&prev_omitted);
    NSDictionary *inf = NSBundle.mainBundle.infoDictionary; NSString *build = [inf[@"CFBundleVersion"] description] ?: @"未知";
    NSDateFormatter *df = [NSDateFormatter new]; df.dateFormat = @"yyyy-MM-dd HH:mm:ss ZZZ";
    char hw[64] = "未知"; size_t hn = sizeof hw; sysctlbyname("hw.machine", hw, &hn, NULL, 0);
    __block float battery_level = -1; __block int bs = 0;
    void (^battery_snapshot)(void) = ^{
        UIDevice *dev = UIDevice.currentDevice; dev.batteryMonitoringEnabled = YES;
        battery_level = dev.batteryLevel; bs = (int)dev.batteryState;
    };
    if (NSThread.isMainThread) battery_snapshot();
    else dispatch_sync(dispatch_get_main_queue(),battery_snapshot);
    NSArray<NSString *> *bst = @[ @"未知", @"放电", @"充电中", @"已充满" ];
    NSDictionary *fsa = [fm attributesOfFileSystemForPath:doc error:nil]; NSProcessInfo *pi = NSProcessInfo.processInfo;
    NSMutableString *h = [NSMutableString string];
    [h appendFormat:@"D2R Mobile Diagnostics\n导出时间 %@\n版本 %@ 构建 %@\n机型 %s 系统 %@\n", [df stringFromDate:NSDate.date], [inf[@"CFBundleShortVersionString"] description] ?: @"未知", build, hw, pi.operatingSystemVersionString];
    [h appendFormat:@"温控档位 %ld 低电量模式 %@ 电量 %.0f%% %@\n", (long)pi.thermalState, pi.lowPowerModeEnabled ? @"开" : @"关", battery_level < 0 ? -1.0 : battery_level * 100, bs >= 0 && bs <= 3 ? bst[bs] : @"未知"];   // 中文一律走 %@：NSString 的 %s 不按 UTF-8 解 | Chinese always goes through %@: NSString's %s does not decode as UTF-8
    [h appendFormat:@"物理内存 %.1f GB 可用存储 %.1f GB 本次已运行 %.0f 分钟\n", pi.physicalMemory / 1073741824.0, [fsa[NSFileSystemFreeSize] doubleValue] / 1073741824.0, -g_t0.timeIntervalSinceNow / 60];
    mtl_metalfx_status graphics = {0}; mtl_metalfx_snapshot(&graphics);
    static const char *const profiles[] = { "Auto", "Eco", "Balanced", "Quality", "Original" };
    static const char *const fallback[] = { "none-or-not-attempted", "unsupported-device", "initialization-failed", "encode-failed" };
    int selected = 0, effective = graphics.profile;
    [h appendFormat:@"\n—— D2R Mobile graphics (applied values; no pending UI changes) ——\nMetalFX enabled=%d selected=%s effective=%s lastEligiblePassActive=%d supported=%d (-1=not-probed) fallback=%s\n", dm_custom_get("metalfx"), selected>=0 && selected<4 ? profiles[selected] : "unknown", effective>=0 && effective<5 ? profiles[effective] : "unknown", graphics.active, graphics.supported, graphics.fallback>=0 && graphics.fallback<4 ? fallback[graphics.fallback] : "unknown"];
    [h appendFormat:@"FPS target=%d renderScale=%.3f lastUpscaleInput=%ux%u lastUpscaleOutput=%ux%u\n", dm_fps_setting(), dm_3d_scale_cur(), graphics.input_width, graphics.input_height, graphics.output_width, graphics.output_height];
    [h appendString:@"Enhanced preferences (applied):"];
    static const char *const custom[] = { "auto_mana", "merc_pot", "drop_light", "map_mark", "item_notes", "icloud", "pot_pick", "drop_haptic", "gem_pick", "store_btn", "loot_filter", "auto_buff", "touch_swap_above" };
    for (unsigned i=0;i<sizeof custom/sizeof *custom;i++) [h appendFormat:@" %s=%d",custom[i],dm_custom_get(custom[i])];
    [h appendString:@"\n"];
    [h appendFormat:@"Log export: at most %lu bytes per log; omitted prefix bytes current=%llu previous=%llu. Original log files are unchanged.\n", (unsigned long)log_limit,cur_omitted,prev_omitted];
    perf_block(h, @"本局关键读数（各取最后一条）", cur);
    perf_block(h, @"上一局关键读数（各取最后一条）", prev);
    NSMutableData *out = [[h dataUsingEncoding:NSUTF8StringEncoding] mutableCopy];
    void (^part)(NSString *, NSData *) = ^(NSString *t, NSData *d) {
        [out appendData:[[NSString stringWithFormat:@"\n\n===== %@（%@）=====\n", t, d ? [NSString stringWithFormat:@"%lu 字节", (unsigned long)d.length] : @"没有这份日志"] dataUsingEncoding:NSUTF8StringEncoding]];
        if (d) [out appendData:d]; };
    part(@"本局日志 boot.txt", cur); part(@"上一局日志 boot.previous.txt", prev);
    NSString *dir = [NSTemporaryDirectory() stringByAppendingPathComponent:@"sharelog"];   // 只放这一份导出：每次重建这个目录（我们自己的，不碰别的） | Hold only this one export: rebuild this directory every time (it is our own; nothing else is touched)
    [fm removeItemAtPath:dir error:nil]; [fm createDirectoryAtPath:dir withIntermediateDirectories:YES attributes:nil error:nil];
    df.dateFormat = @"yyyyMMdd_HHmmss";
    NSString *p = [dir stringByAppendingPathComponent:[NSString stringWithFormat:@(dm_ui("dmShareLogFile")), build, [df stringFromDate:NSDate.date]]];
    // Tail truncation can remove the path that originally identified a name.
    // Read directory names only (never save contents), so recent standalone
    // character-name messages are still redacted from the exported copy.
    NSMutableArray<NSString *> *character_names = [NSMutableArray array];
    for (NSString *root in @[@"save",@"save.work",@"save.test",@"save.test.work"]) {
        NSString *directory = [doc stringByAppendingPathComponent:root];
        for (NSString *name in [fm contentsOfDirectoryAtPath:directory error:nil]) {
            NSString *save = [[directory stringByAppendingPathComponent:name]
                stringByAppendingPathComponent:[name stringByAppendingPathExtension:@"d2s"]];
            if ([fm fileExistsAtPath:save]) [character_names addObject:name];
        }
    }
    NSData *fin = dm_diagnostic_scrub_names(out,NSHomeDirectory(),NSBundle.mainBundle.bundleIdentifier,character_names);
    if (![fin writeToFile:p atomically:YES]) { plog("诊断日志：【红】导出文件写不进 %s", p.UTF8String); return nil; }
    plog("诊断日志：已打包 %s（%lu 字节：本局 %lu、上一局 %s）", p.lastPathComponent.UTF8String, (unsigned long)fin.length, (unsigned long)cur.length,
         prev ? [NSString stringWithFormat:@"%lu", (unsigned long)prev.length].UTF8String : "没有");
    if (getenv("DM_TEST_SHARELOG_CLOSE")) [fin writeToFile:[doc stringByAppendingPathComponent:@"sharelog_test.txt"] atomically:YES];
    // 本地改动：存档导出。把「诊断日志 + 整份存档（Documents/save 整棵树）」打成一个 zip，和上面那份 txt 一起给玩家。
    // 存档目录就是 iCloud 同步的那一份；每个文件按 save/<相对路径> 放进 zip，解开就是一个能直接拖回 App 的 save 文件夹。
    // zip 用存储法（不压缩）。任何一步失败都只是少给一个文件，原来那份 txt 不受影响。
    // Local change: save export. One zip holding the diagnostics file and the whole save tree (Documents/save, the same
    // directory iCloud syncs) is offered next to the txt above. Every file goes in as save/<relative path>, so unpacking
    // yields a save folder that can be dragged straight back into the app. Stored (uncompressed). Any failure only costs
    // that one extra file; the txt above is unaffected.
    NSString *zp = [[p stringByDeletingPathExtension] stringByAppendingPathExtension:@"zip"];
    zs_t z; int zn = 0; long long zb = 0;
    if (zs_open(&z, zp.fileSystemRepresentation)) {
        if (zs_add(&z, p.lastPathComponent.UTF8String, fin.bytes, fin.length)) zn++;
        NSString *sv = [doc stringByAppendingPathComponent:@"save"]; BOOL isd = NO;
        if ([fm fileExistsAtPath:sv isDirectory:&isd] && isd)
            for (NSString *rel in [fm subpathsAtPath:sv]) {
                if (zn >= ZS_MAXENT - 1) break;
                NSString *full = [sv stringByAppendingPathComponent:rel]; BOOL d2 = NO;
                if (![fm fileExistsAtPath:full isDirectory:&d2] || d2) continue;
                NSData *fd = [NSData dataWithContentsOfFile:full options:NSDataReadingMappedIfSafe error:nil];
                if (!fd) continue;
                NSString *nm = [@"save" stringByAppendingPathComponent:rel];
                if (zs_add(&z, nm.UTF8String, fd.bytes, fd.length)) { zn++; zb += (long long)fd.length; } }
        if (zs_close(&z)) plog("诊断日志：存档一起打包 %s（%d 个条目：日志 + 存档 %.1f MB）", zp.lastPathComponent.UTF8String, zn, zb / 1048576.0);
        else plog("诊断日志：【红】存档 zip 收尾失败 %s", zp.UTF8String);
    } else plog("诊断日志：【红】存档 zip 建不出 %s", zp.UTF8String);
    if (getenv("DM_TEST_SHARELOG_CLOSE") && zn) [fm copyItemAtPath:zp toPath:[doc stringByAppendingPathComponent:@"sharelog_test.zip"] error:nil];
    return zn ? @[[NSURL fileURLWithPath:p], [NSURL fileURLWithPath:zp]] : @[[NSURL fileURLWithPath:p]];
}
static UIViewController *top_vc(void) {   // 同 front_ios.m 系统输入框找宿主的顺序：主窗口 → 前台活跃场景的窗口 → 任一窗口，再顺着已弹出的往上找 | Same order as front_ios.m's system input box uses to find its host: main window → a window of the foreground active scene → any window, then walk up through what is already presented
    UIViewController *vc = nil, *alt = nil, *any = nil;
    for (UIScene *s in UIApplication.sharedApplication.connectedScenes)
        if ([s isKindOfClass:UIWindowScene.class]) for (UIWindow *w in ((UIWindowScene *)s).windows) {
            if (!w.rootViewController) continue;
            if (w.isKeyWindow) vc = w.rootViewController;
            else if (s.activationState == UISceneActivationStateForegroundActive && !alt) alt = w.rootViewController;
            else if (!any) any = w.rootViewController; }
    vc = vc ?: alt ?: any; while (vc.presentedViewController) vc = vc.presentedViewController;
    return vc;
}
static _Atomic int g_busy;   // 打包 / 分享面板开着时再点不叠第二个 | Tapping again while packaging / the share sheet is open does not stack a second one
static void on_click(void) {   // 宿主在游戏主线程调：玩家点了「分享诊断日志」 | Called by the host on the game main thread: the player tapped 「分享诊断日志」(Share Diagnostics)
    if (atomic_exchange(&g_busy, 1)) { plog("诊断日志：分享面板还开着，这次点击不再弹"); return; }
    dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0), ^{
        NSArray<NSURL *> *u = build_file();
        dispatch_async(dispatch_get_main_queue(), ^{
            UIViewController *vc = u ? top_vc() : nil;
            if (!vc) { if (u) plog("诊断日志：【红】没找到可弹分享面板的界面"); atomic_store(&g_busy, 0); return; }
            UIActivityViewController *a = [[UIActivityViewController alloc] initWithActivityItems:u applicationActivities:nil];
            a.completionWithItemsHandler = ^(UIActivityType t, BOOL done, NSArray *items, NSError *e) { (void)items;
                plog("诊断日志：分享面板收起（%s%s%s）", done ? "已发到 " : "没发", done ? (t ?: @"未知").UTF8String : "", e ? [NSString stringWithFormat:@"，错误 %@", e.localizedDescription].UTF8String : "");
                atomic_store(&g_busy, 0); };
            UIPopoverPresentationController *pp = a.popoverPresentationController;   // iPad / Mac 上是弹出框：锚在画面正中（暂停菜单所在处），不画箭头 | On iPad / Mac it is a popover: anchored at the center of the picture (where the pause menu is), no arrow drawn
            if (pp) { pp.sourceView = vc.view; pp.sourceRect = CGRectMake(CGRectGetMidX(vc.view.bounds), CGRectGetMidY(vc.view.bounds), 1, 1); pp.permittedArrowDirections = 0; }
            [vc presentViewController:a animated:YES completion:^{
                plog("诊断日志：分享面板已弹出（%s，%s）", pp ? "弹出框" : "底部面板", NSStringFromCGRect(a.view.frame).UTF8String);
                const char *tc = getenv("DM_TEST_SHARELOG_CLOSE"); if (!tc) return;
                dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(atof(tc) * 1e9)), dispatch_get_main_queue(), ^{
                    [a dismissViewControllerAnimated:NO completion:^{ plog("诊断日志：测试开关 DM_TEST_SHARELOG_CLOSE 到点收起分享面板"); atomic_store(&g_busy, 0); }]; });
            }];
        });
    });
}
void dm_sharelog_boot(void) { g_t0 = NSDate.date; dm_sharelog_init(on_click); }   // main.m 起宿主前调一次 | main.m calls this once before starting the host
void dm_sharelog_share(void) { on_click(); }   // 游戏外的界面直接调（资源下载页的「分享诊断日志」按钮，main.m AssetVC） | called directly from screens outside the game (the "Share diagnostic log" button on the asset download page, main.m AssetVC)
