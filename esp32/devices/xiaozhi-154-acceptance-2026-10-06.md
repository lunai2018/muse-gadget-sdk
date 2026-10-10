# Xiaozhi 1.54 Muse Gadget acceptance — 2026-10-06

This report is for the `dev-muse-gadgets` port only. It does not describe or
change the Claude Buddy firmware in the parent repository.

## Hardware and firmware under test

- Board: ESP32-S3-N16R8 Xiaozhi 1.54, 240 x 240 ST7789, CH340K.
- App image: `artifacts/xiaozhi-154/muse-xiaozhi-154-audio-v2-app.bin`.
- App offset: `0x20000`; Wi-Fi, pairing and settings partitions were preserved.
- Image SHA-256:
  `A3EDE8BC21CD57F3BE19CF18A6EA1ACDB99159B510D1FE2EABBED196426C1E86`.

## Results

Acceptance scope note: this build is a successful Muse transport/command-chain
prototype, not yet an accepted interactive Agent terminal. PTT and local menu
buttons work, but Agent content and Agent-action controls are not yet usable as
a complete terminal UI.

| Area | Result | Evidence / limitation |
|---|---|---|
| Boot and flash | PASS | App boots from `ota_0` at `0x20000`; 16 MB flash detected. |
| PSRAM | PASS | 8 MB octal PSRAM detected at 80 MHz. |
| LCD | PASS, quality note | 240 x 240 UI, orientation and controls are usable. The user reports that the grey avatar is visibly less detailed than the AMOLED board; this is a panel/resolution observation, not a driver failure. |
| Physical buttons | PASS | GPIO39 navigates the two-button menu; GPIO10 selects and acts as push-to-talk. |
| Settings persistence | PASS | After a cold boot the log restored volume 85%, microphone gain 18 dB, brightness 100% and sleep 120 s. |
| Local speaker path | PASS | Changing volume produces an audible local chirp. This proves speaker wiring, amplifier, I2S output and volume control. |
| Speaker mute and restore | PASS | The owner confirmed that volume control and the local speaker enable/disable behavior work. |
| Brightness control | PARTIAL | The displayed brightness value and screen appearance change, but the physical backlight change is not pronounced. Cause not yet assigned to hardware or PWM behavior. |
| Microphone electrical path | PASS | Boot self-test at 18 dB measured both channels at -28.6 dBFS, peak 2467, correlation 1.00, effective capture rate 16133 Hz. |
| Microphone cloud path | PASS | A physical push-to-talk recording reached Muse and was transcribed/accepted. |
| Wi-Fi reconnect | PASS | Saved Wi-Fi reconnected after cold boot. In a deliberate AP interruption, LCD showed `RECONNECT` then `NO WIFI`; after AP restoration the board automatically associated and obtained an address without a button press or power cycle. |
| Cloud-session recovery after AP interruption | PARTIAL | After Wi-Fi/IP recovery, control WebSocket, raw tunnel and chat remained down for at least about 55 s with DNS failures. A follow-up serial open reset the board, after which all links and `device.health` recovered. Eventual recovery without reset remains unproven. |
| Muse secure connection | PASS | Noise handshake, control WebSocket, tunnel and `/chat/subscribe` completed. One initial VM-token HTTP 403 recovered automatically on the next credential/connection attempt. |
| Muse text downlink | PARTIAL | Multiple assistant messages reached the board; one controlled `777` reply appeared in both Muse `主聊天` and the small caption area below the LCD avatar. Several earlier replies logged `showing message` but remained invisible; the limited caption area, layout/paging and UI state need investigation. |
| Consecutive cloud turns | PASS | Three consecutive 5.96 s, 6.56 s and 6.04 s recordings were accepted. Muse returned 134, 172 and 37 character messages without a reboot or stuck state. PSRAM returned to approximately 4143 KB after every turn. |
| Chinese/long captions | PARTIAL | Delivery is proven, but several longer replies were invisible on LCD. Short `777` rendered successfully. Wrapping, paging, display lifetime and sleep/state interaction remain unresolved. |
| Explicit Gadget image | PASS | Muse invoked `display.draw_url`; the ESP32 downloaded and rendered a 239 x 178 JPEG (35588 bytes, 672 ms). `display.show_animation` restored the avatar, visible after button wake. |
| Ordinary chat image | EXPECTED WEB/PHONE ONLY | A generated blue circle appeared in Muse Web/phone; no `display.draw_url` reached the board and the LCD stayed on the avatar. Ordinary chat media is not automatic Gadget display transport. |
| Gadget command downlink | PASS | `device.health`, `display.draw_url` and `display.show_animation` reached ESP32 and returned results. A post-cold-boot health call arrived at 44.78 s and returned 352 bytes. |
| Gadget approval | NOT TRIGGERED | Tested Gadget commands executed without an approval card, so Deny could not be exercised. Physical buttons are not wired to Agent approval. |
| Chat/session routing | BOUNDED | Two chats sequentially invoked the same Gadget. Board voice always entered fixed `主聊天`, not the currently open `新旁聊`. Concurrent session control is not proven; the account exposed one VM. |
| Agent terminal interaction | NOT ACCEPTED | Buttons currently provide PTT, local menu selection and wake only. There is no Approve/Deny, Agent action selection, reply navigation/history or chat/session selection on the device. |
| Muse spoken reply | NOT PROVIDED UPSTREAM | The current SDK deliberately requests text modality and paces captions with silence. Muse's former Gadget TTS endpoint was removed. This is not a Xiaozhi speaker fault. |
| Sleep and button wake | PASS | The screen sleeps after the configured delay and a physical button wakes it. |
| OTA rollback validation | PASS | The running image was marked valid after the control WebSocket came up. This does not prove downloading a new OTA image. |
| Runtime stability | PASS SO FAR | Before the deliberate power cycle the heartbeat showed more than 15,800 seconds uptime with Wi-Fi, control WebSocket and raw tunnel up and no reboot loop. |
| Serial log output | PASS | Boot, audio, Wi-Fi, tunnel and heartbeat logs are readable on COM8. |
| Serial bench-command input | ISSUE | Raw `m` was sent on COM8 after boot but no MP3 self-test log appeared. UART receive/bench-command handling is therefore not accepted. Normal physical controls are unaffected. |
| Embedded MP3 reply test | DEFERRED | Deferred by owner. No MP3 success or failure should be inferred from the serial trigger issue. |

## Automated checks

- `python -X utf8 -m unittest esp32.tests.test_muse_serial_chat`: 12 tests,
  one skipped, all executed tests passed.
- The previously produced release firmware completed an ESP-IDF build.
- A broad Windows `unittest discover` run is not a valid product result: many
  upstream host tests require a POSIX `cc`, while others use the Windows GBK
  default instead of UTF-8. Those environment errors are not firmware test
  failures.

## Remaining non-destructive acceptance

- Brightness PWM range/visibility and persistence after a cold boot; the UI
  value changes but the observed backlight range is small.
- Microphone-gain meter response and gain setting persistence.
- Owner confirmation of Chinese glyph quality and long-caption paging.
- Root-cause isolation for why short `777` displays while several longer
  replies reach `showing message` but remain invisible.
- Re-test eventual control/raw/chat recovery after temporary AP loss using a
  capture method that cannot toggle the CH340 reset control lines.

## Intentionally excluded

- MP3/TTS playback, until explicitly resumed.
- Pairing reset or credential erasure; these are destructive and the already
  successful pairing does not need to be repeated.
- Touch, battery telemetry and WS2812: this board port does not enable them.
- A real OTA update download; only rollback validation of the flashed app was
  checked.
