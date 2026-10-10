# Muse Gadget × 小智 1.54 双向链路测试计划

日期：2026-10-06  
项目：`C:\codes\claudebuddy\muse-gadget-sdk-xiaozhi`  
分支：`dev-muse-gadgets`  
当前基线 HEAD：`b139b45064b4dcecf7bfe97e75bc7f99c10c28b6`  
基线固件：`esp32\artifacts\xiaozhi-154\muse-xiaozhi-154-audio-v2-app.bin`  
基线 SHA-256：`A3EDE8BC21CD57F3BE19CF18A6EA1ACDB99159B510D1FE2EABBED196426C1E86`

## 1. 本轮要回答的问题

不再重复一般硬件验收，先对 Muse Gadget 的真实价值做决定性验证：

1. 小智板语音是否能稳定上传 Muse？
2. Muse 的聊天文字回复是否真正下发到 ESP32？
3. Muse 的 Gadget 工具命令 `link.invoke` 是否能下发到 ESP32？
4. `display.draw_url` 是否能在 LCD 显示图片，`display.show_animation` 是否能恢复公仔？
5. 手机上的 Approve/Deny 对 Gadget 工具执行如何生效？小智两个按键能否代替它？
6. 故障是整体网络不通，还是 Muse 的聊天订阅下行与工具控制下行中的某一条不通？

## 2. 当前状态校正

以用户的实机观察为准：

| 项目 | 当前状态 |
|---|---|
| ESP32 语音上传 | **已通过**：Muse 网页/手机端收到语音并进行了计算 |
| Muse 聊天文字→ESP32 | **未通过**：回复只出现在网页/手机端，没有证明 LCD 收到 |
| Muse 聊天图片→ESP32 | **未通过**：普通聊天里生成的图片没有推送到 LCD |
| `device.health` 命令下行 | **未测试** |
| `display.draw_url` 命令下行 | **未测试** |
| `display.show_animation` 命令下行 | **未测试** |
| 真实 TTS/MP3 回复 | **暂停，不纳入本轮** |

现有 `MUSE_XIAOZHI_DEBUG_MEMO_2026-10-06.md` 和
`esp32\devices\xiaozhi-154-acceptance-2026-10-06.md` 中“Muse 文本回复 PASS”的记录
与用户最终实机观察冲突。在本计划取得新证据前，该项不得引用为通过证据。

## 3. Codex 执行红线

1. 只在 `muse-gadget-sdk-xiaozhi` 内工作，不要修改根项目的 Claude Buddy 固件。
2. 本轮先做黑盒测试：**不改代码、不编译、不烧录、不擦除 Flash、不清 NVS、不解绑设备**。
3. 严禁请求 Muse 执行 `device.ota`；这是会写 Flash 和重启的命令。
4. 不做 TTS、MP3、局域网桥、串口 MP3 自检。
5. 不把 LCD 的 `LISTENING`、`WAITING FOR MUSE`、`SPEAKING` 状态动画当成回复文本已下发的证据。
6. 不把 Muse 网页/手机端出现回复当成 ESP32 收到回复的证据。
7. 每次只测一个变量；需要用户说话、按键、手机批准或看屏时，Codex 只给一条明确操作。
8. 不允许用“看起来应该”作结论。每项只能是 `PASS`、`FAIL`、`PARTIAL`、`NOT TRIGGERED`、`NOT RUN`。

## 4. 证据保存位置

每次执行创建新目录，不覆盖旧结果：

`esp32\artifacts\xiaozhi-154\test-runs\YYYYMMDD-HHMM-bidirectional\`

至少保存：

- `baseline.txt`：分支、HEAD、`git status --short`、固件 SHA-256、串口号。
- `serial.log`：从冷启动到全部测试结束的串口日志。
- `observations.md`：用户看到的 LCD、手机批准卡和 Muse 网页结果。
- `result.md`：按本文第 7 节的表格填写结果。

不得在文档中保存 Wi-Fi 密码、SDK token、设备 token 或完整授权头。

## 5. 准备和基线检查

Codex 在 PowerShell 中执行：

```powershell
Set-Location C:\codes\claudebuddy\muse-gadget-sdk-xiaozhi
git status --short --branch
git rev-parse HEAD
Get-FileHash .\esp32\artifacts\xiaozhi-154\muse-xiaozhi-154-audio-v2-app.bin -Algorithm SHA256
python -X utf8 .\esp32\tools\muse\ports.py --list
python -X utf8 .\esp32\tools\muse\ports.py xiaozhi-154
```

注意：

- 工作树已有未提交的小智移植文件，不要清理、reset 或 checkout。
- 串口号会变，不要写死 COM8。
- 先尝试无重启的状态查询：

```powershell
python -X utf8 .\esp32\tools\muse\chat.py --port COMx --status
```

返回 `@status` 则记为“串口 RX/TX 命令链路 PASS”。没有返回只记录为串口控制问题，不得推导为 Muse 云端下行失败。

## 6. 测试顺序

### T0：启动、注册和链路基线

1. Codex 打开串口记录。小智板无独立 Reset，如需完整启动日志，请用户关闭电源开关后再开启。
2. 等待出现稳定心跳，至少连续两次：

```text
hb ... setup=done wifi=up ws=up raw=up
```

3. 记录是否出现 `TLS connected`、控制连接建立、`/chat/subscribe` 建立及异常重连。

**PASS：** `wifi=up ws=up raw=up` 稳定，无重启循环。  
**FAIL：** Wi-Fi、控制 WebSocket 或 raw tunnel 未连上。若 T0 失败，停止后续云端测试，先处理基础连接。

### T1：语音上行复验

1. 用户按住 GPIO10/Talk，说一句唯一口令：

```text
小智上行测试 A，请只回复 DOWNLINK-A。
```

2. 板上松开按键后，串口应出现：

```text
recorded ...
voice note: ...
chat/stream ack: user message ...
```

3. 用户确认 Muse 网页/手机端出现了该句语音的转录。

**PASS：** 上述串口证据和云端转录同时存在。  
此项只证明 ESP32→Muse，不证明 Muse→ESP32。

### T2：聊天文字回复下行

T1 口令发出后，同时检查三处：

1. Muse 网页/手机端是否出现 `DOWNLINK-A`。
2. ESP32 串口是否出现真实回复事件，例如 `reply after ...`，而不是仅有 `chat/stream ack`。
3. LCD 是否真正显示 `DOWNLINK-A`字样。

等待固件自身给出回复或超时结果，不要提前下结论。重复 3 次，口令依次改为 `DOWNLINK-B` 和 `DOWNLINK-C`。

**PASS：** 3 轮中至少 3 轮均有串口回复事件，且 LCD 显示唯一口令。  
**FAIL：** 网页/手机端有完整回复，但 ESP32 没有回复事件，最后出现 `NO REPLY FROM MUSE`。  
**PARTIAL：** 串口有回复事件，但 LCD 没显示或显示错误；这时是本地 UI/分页问题，不是云端没下发。

### T3：Gadget 命令下行——`device.health`

在 Muse 网页/手机端新发一条文字请求，不使用小智麦克风：

```text
请调用已配对 Gadget 的 device.health 工具读取设备状态。不要只用文字解释，必须实际调用该工具。
```

观察：

1. 手机 Muse 是否弹出 Gadget 工具批准卡。
2. 如果弹出，用户选择 Approve。
3. ESP32 串口是否出现：

```text
invoke request: command=device.health
```

4. Muse 是否收到并展示设备返回的健康数据。

**PASS：** 批准后 ESP32 收到 `link.invoke`，Muse 收到命令结果。  
**NOT TRIGGERED：** Muse 只给出文字回答，没有产生批准卡，串口也无 `invoke request`。这只证明 Agent 没有选用工具，不能判定下行网络失败。  
**FAIL：** 手机明确出现并完成 Approve，但 ESP32 始终无 `invoke request`。

### T4：Approve/Deny 边界

再次请求 `device.health`，这次在手机上选择 Deny。

**预期：** ESP32 不应收到 `invoke request: command=device.health`。  
这一测试验证的是 Muse 手机端对 Gadget 命令的审批。小智 GPIO10/GPIO39 当前是 Talk/Select 与 Menu/Down，固件和公开 Gadget 协议没有接入 Agent 的 Approve/Deny 请求，因此本轮不测试用物理按键批准 Agent 操作。

### T5：Gadget 图片命令下行——`display.draw_url`

只在 T3 PASS 后执行。在 Muse 网页/手机端发送：

```text
请必须调用已配对 Gadget 的 display.draw_url，把这张 JPEG 显示在设备屏幕上：https://httpbin.org/image/jpeg
```

1. 手机出现批准卡时选择 Approve。
2. 串口必须出现：

```text
invoke request: command=display.draw_url
```

3. 分开记录三个结果：
   - 命令是否到达 ESP32；
   - ESP32 是否成功下载/解码 JPEG；
   - LCD 是否实际显示图片。

**PASS：** `invoke request` 存在，图片下载/解码成功，LCD 显示图片。  
**PARTIAL：** `invoke request` 存在，但图片下载/解码失败。这已经证明 Muse→ESP32 命令下行正常，后续单独查 URL、TLS、格式或内存，不得把它统称为“Muse 下行失败”。

然后发送：

```text
请必须调用已配对 Gadget 的 display.show_animation，恢复设备公仔动画。
```

**PASS：** 串口出现 `invoke request: command=display.show_animation`，LCD 恢复公仔。

### T6：普通聊天图片与 Gadget 图片命令的对照

在 Muse 聊天中只说“生成一张蓝色圆形图片”，不点名 Gadget 工具。

预期是图片可能只出现在 Muse 聊天端。只有出现
`invoke request: command=display.draw_url` 才能说明图片被作为 Gadget 命令推送。

此项用于证明：“Muse 聊天里生成了图片”和“Muse 命令 Gadget 显示 URL 图片”是两条不同链路。

### T7：会话和 VM 路由边界

本项回答“这块 Gadget 能同时控制几个 session”，不把内部 WebSocket/HTTP 多流误当作多用户会话。

1. 在同一个当前 Muse VM 中准备两个可区分的聊天会话 A 和 B。
2. 在 A 中请求一次 `device.health`，完成 Approve，记录 ESP32 是否收到 invoke。
3. 等 A 完全结束后，在 B 中做同样的请求。两个会话严禁并发，先测路由范围。
4. 再从小智板说一句唯一口令 `BOARD-SESSION-ROUTE`，记录该语音和云端回复出现在 A、B、主聊天还是其他固定入口。
5. 串口日志记录当前 `vm_id`。只有用户账号中已存在第二个可选 VM 时，才可进行“切换 VM 后重复 health”；不得为测试新建或删除 VM。

**PASS 的含义要细分：**

- A 和 B 都能顺序调用同一 Gadget：只证明该 Gadget 命令对两个会话可见，不证明设备能并发管理两个会话。
- 板端语音始终进入一个固定聊天：记为“单一语音会话路由”。
- 当前固件只保持一个选中 VM 的连接；未进行 VM 切换或并发试验时，不得宣称“支持多 VM/多 session 并发控制”。

### T8：聊天回复和命令下行的结合判定

| T2 聊天回复 | T3/T5 命令下行 | 结论 |
|---|---|---|
| PASS | PASS | 设备具备双向聊天与远程 Gadget 控制 |
| FAIL | PASS | 整体网络正常；故障限定在 `/chat/subscribe` 回复投递/关联链路 |
| PASS | FAIL | 聊天下行正常；Gadget 命令的批准、注册或 `link.invoke` 投递异常 |
| FAIL | FAIL，但 T0 PASS | 不能简单归因 Wi-Fi；优先查账号/VM/设备注册与服务端路由 |
| FAIL | NOT TRIGGERED | 命令下行仍无结论；需要先让 Muse 真正生成工具调用 |

### T9：非破坏性稳定性回归

只在 T2–T7 完成或已明确记为 `NOT TRIGGERED`/`NOT RUN` 后做：

1. 冷启动一次，确认 Wi-Fi、配对、音量、亮度和休眠设置未丢失。
2. 临时关闭 Wi-Fi 热点/路由器后恢复，记录 `wifi/ws/raw` 的断开、恢复及耗时。
3. 恢复后重复一次 `device.health`，不只看心跳状态。
4. 不重复已经通过的扬声器、音量、休眠唤醒长时测试，只做一次快速回归。

## 7. 结果表

Codex 执行后必须填完：

| ID | 测试 | 结果 | 关键证据 | 尚未证明的事 |
|---|---|---|---|---|
| T0 | Wi-Fi/控制/raw 链路 |  |  |  |
| T1 | 语音上行 |  |  |  |
| T2-A | 聊天文本下行 A |  |  |  |
| T2-B | 聊天文本下行 B |  |  |  |
| T2-C | 聊天文本下行 C |  |  |  |
| T3 | `device.health` Approve |  |  |  |
| T4 | `device.health` Deny |  |  |  |
| T5-A | `display.draw_url` |  |  |  |
| T5-B | `display.show_animation` |  |  |  |
| T6 | 普通聊天图片对照 |  |  |  |
| T7-A | 会话 A/B 顺序调用 Gadget |  |  |  |
| T7-B | 板端语音会话路由 |  |  |  |
| T7-C | VM 切换（条件允许时） |  |  |  |
| T9 | 冷启动/断网恢复 |  |  |  |

结果报告必须分开：

- 已通过；
- 已失败；
- 部分通过；
- 未触发；
- 未执行。

不得使用“基本可用”概括尚未验证的远程控制。

## 8. 完成后的文档收尾

只在证据保存后更新：

1. `MUSE_XIAOZHI_DEBUG_MEMO_2026-10-06.md`；
2. `esp32\devices\xiaozhi-154-acceptance-2026-10-06.md`。

更新时要删除与实机证据冲突的“Muse 文本回复 PASS”结论，但不得改写或丢失已有固件、烧录、本地音频、麦克风和显示硬件的测试记录。

## 9. 上游已知风险

- 上游 Issue #121 记录了与本板现象高度一致的问题：语音上传和云端回复均成功，网页/手机端有回复，但 Gadget 的 `/chat/subscribe` 没收到回复，最后 `NO REPLY FROM MUSE`。  
  https://github.com/facebookincubator/muse-gadget-sdk/issues/121
- 上游 Issue #108 记录了 `display.draw_url` 可以被 Muse 调用，但手机每次要批准。这证明 Gadget 命令下行与聊天回复下行需要分开测试。  
  https://github.com/facebookincubator/muse-gadget-sdk/issues/108
- 上游 Issue #14 说明公开 Gadget 回复当前为文本，语音输出需要额外 TTS；本轮不把 TTS 混入下行链路判定。  
  https://github.com/facebookincubator/muse-gadget-sdk/issues/14

Issue 是故障参考，不是本板的自动结论。最终必须以本轮串口、LCD、手机批准和 Muse 端四方证据为准。

## 10. 可直接交给 Codex 的执行提示

```text
在 C:\codes\claudebuddy\muse-gadget-sdk-xiaozhi 中，严格按
MUSE_XIAOZHI_BIDIRECTIONAL_TEST_PLAN_2026-10-06.md 执行测试。

本轮只做黑盒验证和保存证据：不改固件代码，不编译，不烧录，不擦除 Flash/NVS，
不解绑，不调用 device.ota，不测 TTS/MP3。保留现有工作树的所有用户改动。

先验证 T0，再分别验证 T2 的 /chat/subscribe 聊天回复下行和 T3/T5 的
link.invoke Gadget 命令下行。需要我说话、按键、在手机 Approve/Deny 或看 LCD 时，
每次只告诉我一个操作。不得把网页回复误记为 ESP32 收到回复。

保存 baseline.txt、serial.log、observations.md、result.md 到新的 test-runs 目录。
最后用 PASS/FAIL/PARTIAL/NOT TRIGGERED/NOT RUN 逐项汇报，并根据 T8 判定是
整体网络、聊天订阅下行、Gadget 命令下行，还是图片下载/本地显示问题。
```
