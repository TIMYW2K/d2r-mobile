# 原生过场动画

移植 augustxxx-debug/d2r-mobile-plus 固定提交 e3756f8942b40eba6c851e881828a5b1bda17fd6 及其已有 VP8 解码后端（MIT，原版权见 LICENSE）。
iOS 默认启用 FFmpeg 7.1.1 的 LGPL 最小静态构建，仅 Matroska/WebM、VP8 与 NV12；构建脚本固定归档 SHA256。设置 DM_MOVIE_FFMPEG=0 可编译旧播放后端。

保留字体和语言设置；配音继续使用当前游戏的 VoiceLanguage。主菜单恢复“电影动画”入口，章节过场走原游戏播放器的事件、PTS、字幕、音频、跳过与循环路径；本功能不恢复改写器已跳过的启动片头。

不分发游戏视频。标准原版 RomFS 中通常已有全局视频；本机资源若缺失或损坏，播放器返回受控失败，不再因打不开文件而直接 die。解码异常交付流尾，背压按稍后重试处理。缺少本地化视频时仍须补齐相应合法游戏资源。

云构建先运行合成 VP8（含无音轨/Opus 音轨）与 FFmpeg NV12 逐字节对照，覆盖元数据、PTS、EOS、seek、循环、背压、停止/跳过、共享读取器释放；再运行 ASan/UBSan。iOS 静态库通过 gen_build.py 自动构建并收进 DM_FRAMEWORKS，兼容实际 reusable Actions，不依赖 run.sh。

App 内 Legal 目录附 FFmpeg 许可与源码/重建说明。真机视频画面、声音、字幕及章节转换仍需安装验收。
