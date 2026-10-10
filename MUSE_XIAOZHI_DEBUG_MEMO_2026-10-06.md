# Muse Gadgets × 小智 1.54 板调试备忘录

日期：2026-10-06  
项目目录：`C:\codes\claudebuddy\muse-gadget-sdk-xiaozhi`  
开发分支：`dev-muse-gadgets`  
记录时 HEAD：`b139b45`  

## 1. 目的与范围

本次工作是在独立的 Muse Gadgets 分支和目录中验证“小智 1.54 英寸板”对 Muse Gadget 固件的适配可能性，不与原有 Claude Desktop Buddy 固件混用。

- 本次实际测试板：ESP32-S3，16 MB Flash，8 MB PSRAM，小智 1.54 英寸 TFT 板。
- Muse 已在手机端开启 Developer Mode，并完成 token、Wi-Fi、云端配对。
- `vendor-source/` 及 `refs/` 只作为只读参考，不得修改。
- 硬件配置以本地小智固件源码为准，不依赖目录中的 PDF。

## 2. 当前推荐固件

本次在实机上实际烧录并验证的是：

`C:\codes\claudebuddy\muse-gadget-sdk-xiaozhi\esp32\artifacts\xiaozhi-154\muse-xiaozhi-154-audio-v2-app.bin`

- 类型：应用分区镜像，不是完整 Flash 镜像
- 烧录地址：`0x20000`
- 文件大小：2,101,248 bytes
- SHA-256：`A3EDE8BC21CD57F3BE19CF18A6EA1ACDB99159B510D1FE2EABBED196426C1E86`
- 固件版本显示：`muse-gadget v999.0.0`
- 编译时间：`Oct 6 2026 10:39:33`
- ELF SHA 前缀：`9899f7d2d`

这是当前应继续测试的“已知可启动、云端可连接、麦克风与本地扬声器链路可工作”的候选版本。

### 重要警告

`muse-xiaozhi-154-full.bin` 是更早生成的整合镜像，尚未合入 audio-v2 修改，不能称为当前最新生产镜像。后续若需要一键整片烧录，应重新构建并合并 audio-v2 后再发布新的 full 镜像。

## 3. 今日生成/保留的固件清单

固件目录：

`C:\codes\claudebuddy\muse-gadget-sdk-xiaozhi\esp32\artifacts\xiaozhi-154`

| 文件 | 大小 | SHA-256 | 用途/状态 |
|---|---:|---|---|
| `bootloader.bin` | 22,624 | `6FFC6152CE3D54F028445E3A41D1DFCD80E9074F6C67F5CFCC2BCF320BFFF3E8` | Bootloader |
| `partition-table.bin` | 3,072 | `FFA95286AED5FF575AEBEA506FDA45007D7066BD5278C3F3A65A960B9D599BB6` | 分区表 |
| `ota_data_initial.bin` | 8,192 | `7D2C7AC4888BFD75CD5F56E8D61F69595121183AFC81556C876732FD3782C62F` | OTA 初始数据 |
| `muse-gadget.bin` | 2,101,248 | `09A134F1DD27BD09536FF8B1450DBD58AA92A383F0CBF7C9F953B209A68E05DC` | 较早的基线应用镜像 |
| `muse-xiaozhi-154-audio-v2-app.bin` | 2,101,248 | `A3EDE8BC21CD57F3BE19CF18A6EA1ACDB99159B510D1FE2EABBED196426C1E86` | **今日实机测试版本，推荐继续使用** |
| `muse-xiaozhi-154-full.bin` | 2,232,320 | `BB3E1DC5BEACC179DC462B99E680D0ED57EDC77F55DD5B2520E0AA18C9AFD7C5` | 较早的整合镜像，不含 audio-v2 |

临时 MP3 自检修改已经撤销。相关构建在真正编译前就因 PowerShell/ESP-IDF 环境识别问题停止，因此没有生成、烧录或保留“MP3 测试固件”。

## 4. GUI 烧录参数

### 已有完整底层镜像时，只更新今日应用固件

ESP32 Flash Download Tool 参数：

- 芯片：ESP32-S3
- 文件：`muse-xiaozhi-154-audio-v2-app.bin`
- 地址：`0x20000`
- 只勾选这一行
- SPI MODE：`DIO`
- SPI SPEED：`80MHz`
- Flash Size：保持原值/不修改
- 不执行 ERASE，以保留 Wi-Fi、配对及 NVS 设置
- 当前串口曾枚举为 `COM8`；Windows 重新插拔后端口号可能变化
- 下载波特率实测 `115200` 可用

进入下载模式：断电，按住 BOOT（GPIO0），重新上电后松开，再点击 START。烧录成功后重新断电上电。

### 从空 Flash 分文件烧录

仅在确实需要全新烧录时使用：

| 地址 | 文件 |
|---:|---|
| `0x0` | `bootloader.bin` |
| `0x10000` | `partition-table.bin` |
| `0x1D000` | `ota_data_initial.bin` |
| `0x20000` | `muse-xiaozhi-154-audio-v2-app.bin` |

不要把应用镜像烧到 `0x0`。旧的 `muse-xiaozhi-154-full.bin` 才是单文件 `0x0` 镜像，但它不是当前 audio-v2 版本。

## 5. 板级资源配置

来源：本地 `vendor-source\xiaozhi-esp32\main\boards\zhengchen\1.54tft-wifi` 和当前板级实现。

### LCD

- ST7789，240×240
- SPI Mode 3，80 MHz
- MOSI：GPIO41
- SCLK：GPIO42
- DC：GPIO40
- RST：GPIO45
- CS：GPIO21
- 背光：GPIO20
- 已按板卡需要配置 RGB/反色显示

### 按键

- GPIO10：Talk / Volume Up / 选择
- GPIO39：Menu / Volume Down / 菜单切换
- GPIO0：BOOT，仅保留为下载模式按键
- 板上没有独立 Reset，测试时用电源开关冷启动

### 音频

- 麦克风：I2S WS GPIO4、SCLK GPIO5、DIN GPIO6
- 麦克风：16 kHz、32-bit、mono-left
- 扬声器：DOUT GPIO7、BCLK GPIO15、WS GPIO16
- 扬声器物理采样率：24 kHz
- Muse 内部音频：16 kHz；板级输出边界做 16→24 kHz 插值
- 当前麦克风增益：18 dB；以 Muse 30 dB 为软件 unity 基准，并在增益前移除 DC

### 当前未启用

- 触摸屏
- WS2812 状态灯
- 电池电量遥测

## 6. 今日测试结果

| 项目 | 结果 | 记录 |
|---|---|---|
| 烧录与冷启动 | 通过 | 从 `ota_0` / `0x20000` 正常启动，无重启循环 |
| Flash / PSRAM | 通过 | 识别 16 MB Flash、8 MB PSRAM，PSRAM 80 MHz |
| LCD 基本显示 | 通过，字幕有间歇问题 | 公仔、状态、菜单、远程图片均可显示；`777` 回复成功显示，但多轮较长回复虽已到板端却未显示文字 |
| LCD 观感 | 记录项 | TFT 上公仔偏灰、像素感明显，不如 AMOLED 细腻；不是启动或通信故障 |
| GPIO39 菜单键 | 通过 | 可切换设置项 |
| GPIO10 选择/Talk 键 | 通过 | 可选择、发起语音输入、唤醒息屏 |
| 本地扬声器链路 | 通过 | 菜单提示音可听见；开关扬声器与音量控制有效 |
| 音量控制 | 通过 | 用户确认音量变化有效，测试中设置过 85% 和 100% |
| 亮度设置 | 部分通过 | 数值/界面有变化，但实际背光明暗差异不明显；尚不能确认是 PWM 范围、驱动方式或硬件观感问题 |
| 自动息屏/按键唤醒 | 通过 | 约 120 秒息屏，按键可恢复 |
| NVS 设置恢复 | 通过 | 冷启动日志恢复过音量 85、麦克风 18、亮度 100、休眠 120 秒；后续菜单操作可能改变最终值，下次先核对 |
| 麦克风本地自检 | 通过 | 18 dB 时约 -28.6 dBFS，peak 2467，DC -269，相关性 1.00 |
| 麦克风到 Muse 云端 | 通过 | 语音成功上传到固定的“主聊天”；T2-C 中 Muse 与 LCD 均正确显示 `777` |
| Wi-Fi 保存与重连 | 通过 | 冷启动后连接已保存网络并获得 IP；未在文档保存 SSID 或密码 |
| Muse Secure Link | 通过 | VM 获取、Noise 握手、控制 WebSocket、隧道、`/chat/subscribe` 均成功 |
| 首次 403 自动恢复 | 通过但需留意 | 某次重启先出现一次 token 403，随后通过 Link 自动恢复并连接成功 |
| OTA 当前镜像确认 | 通过 | 控制 WebSocket 连接后当前 OTA 镜像被标记为有效；这不等于完成了实际 OTA 下载升级测试 |
| 长时间运行 | 初步通过 | 观察到连续运行超过 15,800 秒，Wi-Fi/WS/raw 均保持 up，无重启循环 |
| 连续三轮云端对话 | 通过 | 录音 5.96/6.56/6.04 秒，无削波；回复后 PSRAM 均恢复约 4.143 MB，未见逐轮泄漏或卡死 |
| Muse 文本回复下行 | 部分通过 | 多轮串口收到真实 assistant message；T2-C 的 `777` 在公仔下方的小字幕区成功显示，但此前多轮文字未显示；字幕区域空间有限，需与状态、显示时长和分页一起排查，不是整体网络下行失败 |
| Gadget 命令下行 | 通过 | `device.health`、`display.draw_url`、`display.show_animation` 均由 Muse 下发到 ESP32 并返回结果；冷启动后 health 回归再次通过 |
| Gadget 图片显示 | 通过 | 显式 `display.draw_url` 成功下载并显示 239×178 JPEG；普通聊天生成图片只在 Web/手机出现，不会自动推送到 LCD |
| Gadget Approve/Deny | 未触发 | 多次 health/draw 命令均直接执行，没有出现《批准卡》，因此 Deny 无法测试；板上按键也未接入 Agent 审批协议 |
| 会话路由 | 已明确边界 | 两个聊天可顺序调用同一 Gadget；板端语音固定进入“主聊天”，不跟随手机当前打开的“新旁聊”；未证明并发多会话，仅发现一个 VM |
| Muse 云端语音回复 | 当前设计不提供 | 上游提交改为请求文本回复，并丢弃服务端 TTS 获取；当前 `start_tts()` 用静音 PCM 配合字幕节奏，因此日志中的 reply audio 不代表真实语音 |
| 串口日志 TX | 通过 | COM8 可稳定看到完整启动日志、心跳和会话日志 |
| 串口调试命令 RX | 未通过 | 发送 `m` 没有出现 `mp3 selftest`；只能说明调试命令输入链路未接收，不能据此判定 MP3 解码或扬声器失败 |

连续对话期间的麦克风电平约为：

- RMS：-30.0 / -25.6 / -28.6 dBFS
- Peak：-12.0 / -5.8 / -9.1 dBFS
- 未观察到削波

## 7. 已确认的设计事实

1. ESP32-S3 资源足以承担当前 Gadget 终端工作：显示、按键、I2S 录放音、Wi-Fi、安全连接和云端会话均已同时运行。
2. 当前 Muse 固件不是在 ESP32 本地做 TTS；ESP32 主要负责采集语音、上传、显示文本和播放收到的音频数据。
3. 目前听不到 Muse 的云端回答，并非扬声器硬件失败。菜单提示音已经证明本地扬声器链路可工作；根因是当前 Muse 上游固件选择了文本回复/静音字幕节奏。
4. 本次按用户决定暂停 TTS 和 MP3 工作，不继续引入本地局域网服务，也不改变当前云端直连架构。

## 8. 未完成或明确暂停的项目

### 下次应继续验证

- 较长中文回复为何多次记录到 `showing message` 却未在 LCD 可见；重点检查字幕状态、显示时长、布局/分页和睡眠状态。
- 长文本分页/滚动是否完整，是否能看到结尾。
- AP 关闭/恢复后，Wi-Fi 能自动重新关联并获取 IP；但在后续至少约 55 秒观察期内 Muse 控制、raw tunnel 和聊天订阅仍未恢复，串口持续出现 DNS 解析失败。随后重新打开 CH340 串口意外复位了板子，复位后完整链路约 9 秒恢复。因此“无需复位的云端会话最终恢复”仍未证明。
- 当前 NVS 中最终音量、亮度值；今天菜单测试后可能与冷启动记录不同。
- 背光 GPIO20 的实际 PWM 波形、占空比和有效范围，以区分软件范围与硬件背光电路问题。
- 串口 RX 调试命令为何未被处理；先检查 UART RX、任务创建和命令处理状态，不要把它误判为音频问题。
- 实际 OTA 下载/升级及失败回滚流程。

### 已暂停

- MP3 链路测试。
- Muse 云端 TTS 或本地 TTS 实现。
- 配置局域网 TTS/音频中转设备。

### 不适用/尚未接入

- 触摸功能。
- WS2812 灯效。
- 电池遥测。

为避免破坏当前可用状态，尚未执行清空 NVS、清除 Wi-Fi、清除云端配对等破坏性测试。

## 9. 下次接手建议顺序

1. **先不要重新烧录。** 直接给当前板上电，确认仍能自动连接 Wi-Fi 和 Muse。
2. 打开当前枚举的串口（上次为 COM8，115200），保存完整启动日志。
3. 复现“短文本 `777` 可显示、较长回复不显示”的字幕差异，保存同一轮串口与屏幕录像。
4. 在菜单中核对并记录当前音量、亮度、麦克风增益、休眠时间，再做一次冷启动确认持久化。
5. 使用真正不会切换 CH340 控制线的外部日志方式，复测 AP 恢复后的长期等待，确认 DNS、控制 WebSocket、raw tunnel 和聊天订阅是否最终能在不复位时恢复。
6. 独立排查串口 RX 命令，不修改云端会话和音频主链路。
7. 若恢复 MP3 测试，先修复串口命令入口或使用明确的启动时一次性自检；测试完成后必须移除自动播放代码。
8. 若恢复 TTS，先重新确认 Muse 当前服务端协议是否能返回音频；不要默认需要 ESP32 本地 TTS，也不要先部署局域网服务。
9. 在准备发布“一键整合镜像”时，用 audio-v2 重新构建并合并 full image，重新记录大小、SHA-256 和实机全擦除烧录结果。

## 10. 构建与测试环境备注

- ESP-IDF：`C:\esp\v6.0.1\esp-idf`，版本 v6.0.1。
- 已成功产出的 release 构建目录：`esp32\build-muse-xiaozhi-154-release`。
- 今日 audio-v2 构建和板级单元测试此前已经成功。
- `python -X utf8 -m unittest esp32.tests.test_muse_serial_chat`：12 项测试，1 项跳过，其余通过。
- 全量 `unittest discover` 曾出现 93 个错误、22 个跳过，主要是 Windows 默认 GBK 解码和缺少 POSIX `cc`，不能记为固件产品失败。
- 随后的普通 PowerShell 构建尝试在 ESP-IDF 导出阶段报 `Support for platform 'Windows-' hasn't been added yet`，尚未进入编译；下次应使用已经正确初始化的 ESP-IDF PowerShell/终端环境。

## 11. 关键文件

- 板级实现：`esp32\components\muse\boards\board_xiaozhi_154.c`
- 板级配置：`esp32\devices\sdkconfig.muse-xiaozhi-154`
- 板卡说明：`esp32\devices\xiaozhi-154.md`
- 详细验收记录：`esp32\devices\xiaozhi-154-acceptance-2026-10-06.md`
- 固件及烧录说明：`esp32\artifacts\xiaozhi-154\FLASH-GUIDE.md`
- 串口工具：`esp32\tools\muse\chat.py`、`board.sh`、`ports.py`
- 串口测试：`esp32\tests\test_muse_serial_chat.py`
- 只读厂商参考：`vendor-source\xiaozhi-esp32\main\boards\zhengchen\1.54tft-wifi`

## 12. 当前结论

小智 1.54 板目前只证明了 Muse Gadget 的基础协议和硬件链路，尚不能验收为可用的 Agent 终端。语音可上传固定“主聊天”，但 Agent 内容输出不可靠：只有一次短文本 `777` 在公仔下方的小区域显示，多轮较长回复虽已到板端却不可见；普通聊天图片不会自动显示，只有显式 `display.draw_url` 命令可以画图。两个实体按键目前只承担 PTT、菜单选择和唤醒，不能 Approve/Deny、选择 Agent 操作、翻阅回复或切换聊天会话。

当前结论应表述为“远程命令链路原型通过”，而不是“桌面终端完成”。要成为可用 Agent 终端，至少还要补齐稳定且足够大的内容显示、长文本翻页/历史查看、Agent 操作卡片与按键响应、明确的会话状态，以及必要时的语音输出。其他缺口包括 SDK token、NVS 加密、亮度观感、串口 RX 调试入口和主动断网恢复验证。继续开发时应以现有 audio-v2 应用镜像为基线，避免误刷旧 full 镜像或清除现有配对数据。

## 13. 2026-10-07 audio-v3 追加验证与候选修正

### 已上板并由用户确认（audio-v3）

- 应用镜像从 `0x20000` 启动，保留了原有 Wi-Fi、Muse 配对和 NVS。
- 冷启动后 Wi-Fi、控制 WebSocket、raw tunnel、`/chat/subscribe` 全部恢复；OTA 健康检查通过，观察期内无 panic 或重启。
- Muse 网页端回复 `8888` 时，ESP32 LCD 同步显示 `8888`。这直接证明 Muse → ESP32 文本下行成功；先前“整体下行失败”的结论撤销。
- 中文回复可在 LCD 正确绘制，CJK 字体链路通过。
- GPIO39 在菜单中长按约 0.6～1 秒可退出菜单，新增的两键 `Back` 操作通过。
- 当前回复仍显示在公仔下方的窄字幕区域。照片证明这是 UI 布局问题，不是网络、会话路由或中文字库问题。

### 已定位根因

- 240×240 Xiaozhi 被 `muse_ui.c` 判定为紧凑屏幕（`s_small`）。
- 紧凑布局在创建全屏 `s_reply_lbl` 前提前返回，因此即使状态逻辑选择 `ANSWER_READ`，实际也只能继续使用底部 `s_caption_lbl`。
- 串口可输出日志但不响应 `>status`/chat 测试命令：`MUSE_CONSOLE_UART` 的隐藏 Kconfig 默认条件未包含 `MUSE_BOARD_XIAOZHI_154`，输入任务错误地监听未接出的 USB Serial/JTAG，而不是 CH340K 后面的 UART0。

### 已改源码并生成候选固件、尚未上板

- 为紧凑屏幕增加全文阅读页：保留顶部连接状态，回复期间隐藏公仔、状态名和按键图标，正文约使用 224×198 像素；中文分页目标约 14 字×11 行；回复结束后恢复公仔。
- `MUSE_CONSOLE_UART` 默认条件加入 Xiaozhi，使后续版本可使用串口 `status/chat/按键模拟` 做自动验收。
- 相关静态回归测试：17 项通过，2 项按环境跳过。
- 已从正确的 release 目录完成全量构建并生成签名应用镜像，但尚未烧录，不能记为实机通过。
- 候选文件：`esp32\artifacts\xiaozhi-154\muse-xiaozhi-154-audio-v4-app.bin`
- 大小：`2,953,216` 字节；SHA-256：`1D2130D71A031DDE50703031FE4219118963CF4BBFF38A3E97BD0C7B5E0D92CF`
- 构建配置复核：CJK 字体启用、UART 控制台启用、测试 token 存在且长度为 48（备忘录不记录 token 内容）。

### 构建状态与注意事项

- 一次构建误用了旧的 `build-muse-xiaozhi-154` 目录；该目录本来就没有 SDK token。已立即停止，没有打包或刷入设备，不能据此判断 token 被 ESP-IDF 清除。
- 正确的 `build-muse-xiaozhi-154-release/sdkconfig` 仍保存 48 字符测试 token，并启用了 CJK。当前板上的 audio-v3 云端连接正常。
- 下一个候选镜像必须继续使用 release 构建目录和现有测试 token；完成验证后按用户决定轮换 token。仍只刷应用区 `0x20000`，保留配对数据。

### 下一轮上板验收

1. 中文短回复进入全文页，公仔隐藏，正文不再挤在底部字幕带。
2. 超过一页的中英文混合回复正确换行、分页，能看到结尾。
3. 回复结束或退出后恢复公仔，菜单与 PTT 不受影响。
4. 串口 `>status` 和 typed chat 可用，用它核对下行内容与 LCD 同轮显示。
5. AP 关闭/恢复后，不重启设备验证 DNS 清理、控制 WebSocket、raw tunnel 和 chat subscribe 自动恢复。

## 14. 2026-10-07 audio-v4 实机结果与 audio-v5 候选

### audio-v4 已由用户实机确认

- 回复期间公仔会隐藏，回复结束后公仔恢复。
- 发送等待状态和 Muse 回复都进入大面积信息页，不再挤在公仔下方的小字幕带。
- 中文能够显示；COM7 `status` 命令已恢复，Wi-Fi、Hatch 和 Link 状态均可读取。
- 长中文回复没有观察到结尾“测试结束”，因此长文本分页/末页停留仍未通过，audio-v4 不能作为最终验收版。

### audio-v5 已构建、尚未上板

- 将 `MUSE_CAPTION_MAX` 从 400 增至 512。240×240 中文页按 14 字×11 行计算，UTF-8 正文连同换行最多约 472 字节；原缓冲存在末行装不下的风险。
- 将无云端语音时的最终阅读页停留从 2 秒延长到 5 秒，之后才恢复公仔。
- 静态回归测试：17 项通过，2 项按环境跳过；签名镜像结构和校验均有效。
- 文件：`esp32\artifacts\xiaozhi-154\muse-xiaozhi-154-audio-v5-app.bin`
- 大小：`2,953,216` 字节；SHA-256：`BC14C05C60703A095EAB382EE219405F86F6B832741432EAB46D6F243E980E51`
- 仍只允许从 `0x20000` 写应用镜像，保留 NVS、Wi-Fi 和 Muse 配对。

### audio-v5 实机追加结果

- 已刷入并正常恢复 Wi-Fi、Hatch、Link；COM7 日志持续稳定，无重启或 panic。
- 用户确认长回复能够自动换页，末页约停留 5 秒，随后恢复公仔。
- 实测回复为 673 字节；日志记录无 TTS 时用静音计时推进字幕，共模拟 `47.06s of audio`，整轮 `52.75s`。因此“换页慢”是当前每秒 16 字节阅读节拍造成的，中文 UTF-8 每字通常占 3 字节，实际仅约 5 个汉字/秒，不是网络卡顿。
- 当前没有实体键手动翻页；音量键短按仍进入/移动菜单。触摸屏并非必要条件，下一候选可在回复阅读态复用音量键短按“下一页”、长按“退出阅读”，并保留 PTT 键打断/新提问。

### audio-v6 已构建、尚未上板

- 无 TTS 阅读速度改为按 UTF-8 可见字符计时，续字节不再重复计数；中文不会再因为每字约 3 字节而变成约 5 字/秒。
- 全屏回复阅读态下，音量键短按请求下一页；长按约 0.6 秒取消当前回复并恢复公仔。非阅读态仍保持原菜单操作。
- PTT 键行为不变：回复期间按下会打断当前回复并开始新语音轮次。
- 保留 audio-v5 的 512 字节中文页缓冲及末页 5 秒停留。
- 静态回归测试：18 项通过，2 项按环境跳过；完整 ESP-IDF 增量构建成功，镜像校验有效。
- 文件：`esp32\artifacts\xiaozhi-154\muse-xiaozhi-154-audio-v6-app.bin`
- 大小：`2,953,216` 字节；SHA-256：`B4850533F806112CBF35A86578292CCB3079EF075C19710A8D0B847AA6C2F16D`
- 页码角标尚未加入 v6；先独立验收按键导航和速度，避免页码布局同时干扰正文区域。

### audio-v6 实机验收结果

- 用户确认自动翻页约每 5 秒一页，速度合适。
- 回复阅读态下，音量键短按能够立即翻到下一页；日志记录到 `reply next page`，未误开设置菜单。
- 音量键长按能够退出阅读并恢复公仔；日志记录到 `reply dismissed from the reader`，会话随后由固件主动取消并释放 PSRAM。
- 测试回复日志：Muse 返回 `1109 bytes, 345 chars`。这证明 v6 已按可见字符而不是 UTF-8 字节计算阅读时长。
- 测试过程中 Wi-Fi、WebSocket、raw tunnel 持续在线，无 panic、复位或内存持续下降。
- v6 的核心阅读交互可记为通过；尚未加入页码角标，上一页/历史回看也不在本版范围内。

## 15. 2026-10-08 audio-v7 页码候选

- 在 240×240 全屏回复页右下角增加独立 `当前页/总页数` 角标；只有多页回复才显示，单页回复不显示。
- 页码不写入回复正文，也不占用 Muse 下行文本缓冲；正文区域底部预留一行，避免角标覆盖文字。
- 页数与正文使用同一套换行和“一行重叠”分页算法；自动换页或音量键短按翻页时同步更新。
- 退出阅读或回复结束进入 IDLE 时清空页码状态并恢复公仔。
- 保留 v6 已实机通过的行为：约 5 秒自动翻页、音量键短按下一页、长按退出、PTT 打断/新提问。
- 主机回归测试：19 项通过，2 项按环境跳过；新增三页重叠分页的 `1/3`、`2/3`、`3/3` 行为验证。
- 完整 ESP-IDF 构建成功，签名镜像结构和校验有效；尚未上板，不能记为实机通过。
- 文件：`esp32\artifacts\xiaozhi-154\muse-xiaozhi-154-audio-v7-app.bin`
- 大小：`2,953,216` 字节；SHA-256：`8A039176B9CF5C4B2E0DF8171B5C28DBA101146D257A075BF1F71B2421350488`
- 镜像：ESP32-S3、16MB Flash、80MHz、DIO；仍只从 `0x20000` 写应用镜像，保留 NVS、Wi-Fi 和 Muse 配对。

### audio-v7 上板验收点

1. 单页短回复不显示页码。
2. 长回复右下角显示例如 `1/3`，自动翻页和音量键短按时依次变化。
3. 页码不遮挡正文，末页文字完整可见。
4. 音量键长按退出后页码消失并恢复公仔。

### audio-v7 实机验收结果

- 2026-10-08 已刷入小智 1.54 板并完成真实板端语音测试；本次重启后调试串口枚举为 `COM8`。
- Wi-Fi 自动恢复为 `connected`，Hatch 为 `Connected`，Link 为 `Online`，无需重新配对。
- 用户从板上录制 9.86 秒语音，云端确认消息并返回 634 字节、214 个可见字符的中文回复。
- LCD 正确显示两页回复及 `1/2`、`2/2` 页码。
- 音量键短按能够翻页，长按能够退出阅读并恢复公仔。
- 测试期间 Wi-Fi、WebSocket、raw tunnel 持续在线；无 panic、复位或内存持续下降。
- 注意：`tools/muse/chat.py` 的 typed-chat 回复按官方设计只回传串口，不进入语音任务和 LCD，不能用它验收回复页 UI。LCD 验收必须由板上语音发起，或另行实现显式显示测试命令。
- 结论：audio-v7 的大屏文本阅读、中文显示、自动/手动翻页、页码和退出交互均已实机通过，可作为下一阶段 Muse 功能验收基线。

## 16. 2026-10-08 TTS 与 Approve/Deny 边界核实

### 小智原固件的语音输出

- 小智固件没有绑定某一家 TTS，也没有在 ESP32 本地合成语音。
- 小智通过 WebSocket/MQTT 音频会话上传 Opus 麦克风帧；服务端完成 ASR、LLM 与 TTS 后，以 `tts start/stop` JSON 加二进制 Opus 音频帧下发。
- ESP32 收到 Opus 后放入解码队列，解码为 PCM；采样率不匹配时重采样，最后交给音频 codec/I2S 和扬声器播放。
- 因而“原来小智能说话”证明扬声器和 codec 硬件正常，也证明原小智云端会返回合成音频；仅从固件不能判断官方云端具体选用了哪一家 TTS，TTS 提供商和音色属于服务端配置。
- 当前 Muse SDK 则明确只下发文本。`start_tts()` 只是接入点，要求开发者自行把回复文本发送给任意 TTS API，再将返回的 MP3 交给已有 minimp3/扬声器链路。

### Approve/Deny

- 当前官方 ESP32 SDK 的 Gadget 控制协议是 `link.register` 注册命令、Muse 发送 `link.invoke`、设备返回结果；源码没有 approval/approve/deny 卡片事件或实体按键结果协议。
- 2026-10-08 通过 COM8 直接询问当前 Muse 并让其检查在线 Gadget。Muse 报告当前可见命令为 `device.discover`、`device.health`、`device.ota`、`display.draw_url`、`display.show_animation`，没有任何审批或输入类工具。
- Muse 说明，未授权命令可能出现的审批属于手机端对命令权限的一次性系统弹窗，不是 Agent 可下发自定义文案并从 Gadget 收回按键选择的原生卡片机制。
- Muse 原始回答保存为：`esp32\artifacts\xiaozhi-154\muse-approval-capability-response.txt`。
- 可以自行新增 `approval.request` 命令：Muse 调用后设备显示问题，短按/长按分别返回 approve/deny。但它只能作为“自定义二次确认工具”；除非 Muse 云端明确将结果接入敏感操作的执行门控，否则不能冒充平台原生权限审批。

## 17. 2026-10-08 audio-v8 自定义 Approve/Deny 候选

### 已实现、尚未上板

- 新增 Gadget 命令 `approval.request`，必填 `prompt`，可选 `detail` 和 `timeout_s`（5～120 秒，默认 30 秒）。
- Muse 调用后，小智屏幕会自动唤醒并显示独立全屏确认卡；确认卡不依赖 LISTENING/THINKING/SPEAKING 状态，因此语音状态更新不会覆盖它。
- 讲话键短按返回 `approved=true, reason=approved`；音量键短按返回 `approved=false, reason=denied`。
- 超时返回 `approved=false, reason=timeout`；一次只允许一张确认卡，第二个请求返回 `busy`。
- 卡片激活期间屏蔽 PTT、菜单和回复翻页；作出决定后的松键也被吞掉，避免误启动录音或打开菜单。
- 这是 Gadget 自定义二次确认，不是 Muse 平台原生权限审批。真正的执行门控仍需 Muse 在危险操作前主动调用 `approval.request`，并仅在 `approved=true` 时继续。
- 主机相关回归测试：20 项通过，2 项按环境跳过。
- ESP-IDF v6.0.1 完整构建及竞态修正后的增量构建均成功；签名应用大小 `0x2d1000`，4 MiB 应用分区剩余 `0x12f000`（30%）。
- 文件：`esp32\artifacts\xiaozhi-154\muse-xiaozhi-154-audio-v8-approval-app.bin`
- 大小：`2,953,216` 字节；SHA-256：`56CD9642D7CF4649F51E772956A38CF32C9DE2E32ECFFCDBE0DD3FD5F5637C59`
- 烧录参数仍为 ESP32-S3、16MB Flash、DIO、80MHz，仅将此签名应用镜像写入 `0x20000`；保留 NVS、Wi-Fi 和 Muse 配对。

### v8 上板验收点

1. 刷机重启并恢复 `Wi-Fi ready` 后，让 Muse 明确调用 `approval.request`，确认 240×240 LCD 显示全屏问题和两侧按键提示。
2. 第一轮短按讲话键，Muse 应收到 `approved=true`；松键不得启动录音。
3. 第二轮短按音量键，Muse 应收到 `approved=false, reason=denied`；不得进入菜单或翻页。
4. 第三轮设置 5 秒超时且不按键，结果必须为 `approved=false, reason=timeout`，随后恢复公仔。
5. 确认卡期间屏幕不得自动息屏；中文 `prompt/detail` 应正常换行显示。

### v8 实机验收结果

- 已刷入小智 1.54 板，重启后保留 Wi-Fi 和 Muse 配对；状态恢复为 Wi-Fi `connected`、Hatch `Connected`、Link `Online`。
- 首轮 30 秒确认卡未按键，设备返回 `approved=false, reason=timeout` 并恢复公仔，证明超时默认拒绝有效；这不是网络失败或 Muse 主动取消。
- 第二轮设置 120 秒：确认卡出现后短按讲话键，卡片立即消失并恢复公仔，没有误启动录音；Muse 收到 `approved=true, reason=approved`。
- 第三轮设置 120 秒：确认卡出现后短按音量键，卡片立即消失并恢复公仔，没有误进入菜单或翻页；Muse 收到 `approved=false, reason=denied`。
- 结论：`approval.request` 的下行显示、物理批准、物理拒绝、超时默认拒绝及松键隔离均已实机通过。
- 使用注意：`timeout_s` 从 Muse 发出工具调用时开始计时，用户看到卡片前已消耗少量云端调度/传输时间。正式交互建议使用 60～120 秒，不建议沿用测试默认的 30 秒。

### Muse workspace 持久对接结果

- 新增英文 Skill：`skills/gadget-physical-approval/SKILL.md`，并加入本仓库 `skills/CATALOG.md`；格式验证通过。
- 用户已通过 Muse 主聊天将 Skill 持久安装到 `/home/hatch/workspace/skills/gadget-physical-approval/SKILL.md`。
- Muse 已在 `/home/hatch/AGENTS.md` 新增 `Gadget physical approval` 规则，并读回两个文件验证；安装过程没有调用 `approval.request`。
- 随后的 dry-run 让 Muse 模拟永久删除 `approval-test-only`。Muse 自动选择 `approval.request`，实体批准后得到 `approved=true, reason=approved`，并按 dry-run 约束没有删除或修改任何内容。
- `device.describe` 现在列出 6 个 Gadget 命令，其中包括 `approval.request`，描述明确为全屏卡片和实体按键等待。这表示 v8 自定义命令已通过标准 `link.register` 成功成为该设备的原生可调用命令；不表示上游官方固件原本内置了审批命令。
- 当前结论：ESP32 命令注册、Muse Skill 自动路由、实体批准返回和拒绝执行约束均已贯通。Skill/AGENTS 仍是 Agent 行为约束，不是执行端不可绕过的安全锁。
