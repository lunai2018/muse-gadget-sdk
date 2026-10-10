# Muse Gadget Xiaozhi v23 milestone summary

Date: 2026-10-10 (America/New_York)

Repository: `muse-gadget-sdk-xiaozhi`

Development branch: `dev-muse-gadgets`

Upstream base: `facebookincubator/muse-gadget-sdk` at `b139b45`

Hardware: Xiaozhi 1.54, ESP32-S3-N16R8 (16 MB flash / 8 MB PSRAM)

## 1. Milestone result

The Xiaozhi board now works as a two-way Muse Gadget endpoint rather than only
an uplink microphone. The v23 acceptance cycle passed the critical paths:

- Wi-Fi provisioning, Muse pairing, reconnect and health reporting.
- Voice-note upload from the physical TALK key.
- Routed Muse replies shown as full-screen text, including Chinese glyphs.
- Multi-page long text with about five-second auto paging, page counters,
  VOLUME short-press for next page and long-press to return to the avatar.
- Azure Speech text-to-speech played through the onboard speaker.
- Barge-in: TALK stops current playback and the same press starts recording;
  release sends one voice note without entering the menu.
- Full-screen physical approval cards with TALK=Approve and VOLUME=Deny.
- A sleeping/black LCD is awakened by both routed reply events and
  `approval.request`; Approve, Deny and Timeout were all physically verified.

The previous blocking defect, "approval.request does not wake the sleeping
display", is closed by v23. The current packaging verdict is PASS for the v23
development candidate.

## 2. Hardware configuration

The pin map was derived from the known-working Xiaozhi firmware, not from the
reference PDF:

| Function | Configuration |
|---|---|
| LCD | ST7789 240 x 240, SPI mode 3, 80 MHz, RGB order, inversion enabled |
| LCD MOSI / SCLK / DC / RST / CS / BL | GPIO 41 / 42 / 40 / 45 / 21 / 20 |
| TALK / VOLUME | GPIO 10 / 39, active low |
| Microphone WS / SCLK / DIN | GPIO 4 / 5 / 6 |
| Speaker DOUT / BCLK / WS | GPIO 7 / 15 / 16 |
| UART TX / RX | GPIO 43 / 44 |
| ROM download | GPIO 0, reserved |

Audio capture is 16 kHz. Speaker output uses the board's verified 24 kHz clock,
with Muse audio converted at the board boundary. The current board has no touch
input enabled and no separate reset button; power cycling is the normal reset.

## 3. Software and service configuration

- ESP-IDF: 6.0.1.
- Board overlay: `esp32/devices/sdkconfig.muse-xiaozhi-154`.
- Board implementation: `esp32/components/muse/boards/board_xiaozhi_154.c`.
- Cloud TTS: Azure Speech, validated with `en-US-AvaNeural` and
  `audio-16khz-128kbitrate-mono-mp3`.
- Azure authentication: the cloud connector injects the key; the setup tool
  provisions device settings in NVS. No plaintext Azure key belongs in Git.
- Muse developer token: local-only provisioning data (`.sdk-token` is ignored).
- Physical approval integration: `approval.request` plus the
  `skills/gadget-physical-approval/` operating instructions.

Muse routing semantics discovered during testing are important:

1. A Gadget-originated voice note opens a routed turn; the assistant reply is
   delivered to that Gadget and can wake its LCD, display text and play TTS.
2. Explicit device commands such as `approval.request` are always addressed to
   the Gadget and work independently of an active chat turn.
3. A standalone Muse app message without an active Gadget turn is not addressed
   to the Gadget. Lack of display/audio in that case is not a firmware failure.

## 4. Debug progression

| Stage | Main finding and fix |
|---|---|
| Initial port | LCD, buttons, Wi-Fi and cloud pairing worked; speaker pin/rate configuration did not. |
| Audio v2-v7 | Aligned raw-I2S pins and slot format with Xiaozhi; confirmed microphone upload and speaker path. |
| v8-v13 | Added physical approval and Azure MP3/TTS delivery; separated text display from player lifecycle. |
| v14-v18 | Added reply wake, CJK rendering, menu isolation and correct TALK press/release behavior. |
| v19-v20 | Recovered from long replies and fixed serial framing so multi-page text completed reliably. |
| v21-v22 | Added delayed reply wake and THINKING/sleep recovery for active voice turns. |
| v23 | Unified approval wake with a display-visible handshake; timeout starts only after the card is visible. |

The v23 approval flow wakes the panel before rendering, prevents physical input
until the card is visible, and distinguishes a card that never became visible
from an ordinary user timeout. After Approve, Deny or Timeout, the avatar is
restored without entering recording or the menu.

## 5. Acceptance evidence

Physical checks completed on the actual board:

| Test | Result |
|---|---|
| Connection/status and reconnect | PASS |
| Voice uplink and routed text reply | PASS |
| Chinese text | PASS |
| Long text, auto/manual paging and exit | PASS |
| Azure TTS and final sentence completion | PASS |
| TALK barge-in and single-send release | PASS |
| Approval while awake: Approve / Deny | PASS |
| Approval from black LCD: Approve / Deny / Timeout | PASS |
| Return to avatar after approval | PASS |

Automated checks at this milestone:

- `python -m unittest tests.test_xiaozhi_reply_recovery_contract`: 16/16 PASS.
- Full Python discovery: 140 tests discovered; 87 passed, 25 skipped and 28
  could not run because this Windows environment has no host `cc/c++` compiler.
  No v23 recovery-contract test failed.
- The validated v23 ESP-IDF build completed before physical acceptance. The app
  image size was `0x2d1000`, leaving about 30 percent of its app partition free.

## 6. Validated firmware artifact

File:

`esp32/artifacts/xiaozhi-154/muse-xiaozhi-154-audio-v23-approval-sleep-wake-app.bin`

Size: 2,953,216 bytes

SHA-256: `22A17745F8E5AB1BCCF438622CD267291BC9E961A3ACA961E8B29A2285943FA4`

Flash this application image at `0x20000`, DIO, 80 MHz, without erase. See the
adjacent `FLASH-GUIDE.md` for the complete GUI procedure.

## 7. Cleanup and repository boundaries

This work is intentionally isolated in `dev-muse-gadgets`. It must not be mixed
with the outer Claude Buddy `case-design` branch or pushed to the official
Facebook upstream remote.

For this milestone the repository keeps source, tests, design/debug notes and
only the validated v23 application image. Generated ESP-IDF build trees,
pytest caches, the downloaded vendor-source checkout, v2-v22 binaries and old
serial/screenshot test-run artifacts are removed because they are reproducible
or superseded. Factory backups remain local and ignored.

## 8. Remaining release tasks

- The device currently reports firmware `999.0.0`; `v23` is a development
  artifact label, not yet a formal semantic release version.
- Rotate the test Muse developer token immediately before a public/final build.
- Keep Azure and Muse credentials out of source, logs and release assets.
- Battery telemetry is unavailable on this board (`battery=-1`).
- Brightness values change in software, but the observed physical backlight
  change is subtle and may be limited by the board hardware.

These items do not invalidate the v23 functional milestone, but the first two
must be completed before a public production package.
