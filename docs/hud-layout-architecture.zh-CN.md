# HUD 几何、布局编辑与地图绘制

本文件记录 iOS 壳与游戏本体之间的布局边界，供排查竖屏错位使用。

- `dm/common/dm_hud_geometry.h`：壳与游戏层共用的 HUD 位移和坐标转换。参数标明 UIKit 点、实际像素、以界面带高度为 2160 的游戏布局单位。
- `dm/ios/Sources/dm_touch_controls.c`：手势命中区域；`DmTouchOverlay.m`：UIKit 美术与文字绘制。这两处共享 HUD 位移，不持有游戏控件指针。
- `dm/host/dm_cursor.c`：游戏 UI 树的布局调整、方向状态与小地图位置发布。所有控件读写在既有游戏线程和位置守卫内进行。
- `dm/gfx/nvn/nvn_minimap_shift.h`：根据视口和裁剪矩形识别地图绘制；`nvn_mtl.m` 只在识别成功时移动对应绘制。
- `dm/ios/tools/gen_fe_rightpanel.py` 与 `touch_layout.py`：从原始资源生成角色列表贴图和触屏布局。焦点高亮和选中图是独立素材，必须分别验证。

22.1 的诊断中，地图目标框与实际绘制不一致，而且日志里没有地图绘制上移的确认信息。当前代码仅在非空绘制已编码后确认上移，并让红框与触摸区域跟随该状态；这不能证明地图本体的所有绘制已移到目标位置。新加入的“地图裁剪样本”最多记录 24 种与预期区域相交的剪裁，可供下一次真机诊断核对，记录本身不改变绘制匹配。文字阴影重排也尚待真机验证。苹果文档建议通过 Xcode 的 GPU Frame Capture 记录实际渲染命令；核对地图和文字绘制各自的 viewport、scissor、投影矩阵及绘制顺序之后再修改匹配规则。

现有 `C:/Diablo II Mobile/ui-editor/index.html` 预览的是从已发布 IPA 抽取的快照，源码中的未发布改动不会自动出现。`ui-editor/tools/extract_ipa_layouts.py` 将指定 IPA 的布局提取到新目录；`build_data.py --overlay` 使用指定快照重建数据。导出需要人工核对和接入构建，不直接改游戏存档或安装包。

参考资料：

- https://developer.apple.com/documentation/metal/mtlrendercommandencoder/setviewports(_:)
- https://developer.apple.com/documentation/xcode/capturing-a-metal-workload-in-xcode
- https://github.com/D2R-Reimagined/reimagined-d2r-mod-studio#ui-designer

22.1 实机截图底栏核对（1206×2622，界面带高 1334）：两个球液中心相对底图中心约右偏 30px；前两瓶药水约右偏 22–23px。代码中的竖屏校准量结合了 22.1 原先按各自缩放除法计算的偏移；这只是针对该截图的计算预测，设备效果仍需验证。
