# Xiaozhi 1.54 custom board

This local port targets the tested ESP32-S3-N16R8 Xiaozhi board with a 1.54"
240×240 ST7789 display, raw-I2S microphone and speaker, and a CH340K UART
bridge. It deliberately does not enable touch or the optional WS2812.

## Verified hardware map

The display and buttons come from the previously running local Buddy firmware,
not from the generic board PDF. The complete pin map and audio configuration
match `main/boards/zhengchen/1.54tft-wifi` in the locally saved upstream
`78/xiaozhi-esp32` firmware.

| Function | GPIO |
|---|---:|
| LCD MOSI / SCLK / DC / RST / CS / BL | 41 / 42 / 40 / 45 / 21 / 20 |
| Talk (volume up) / menu (volume down) | 10 / 39 |
| ROM download button | 0 (reserved, not consumed by Muse) |
| Microphone WS / SCLK / DIN | 4 / 5 / 6 |
| Speaker DOUT / BCLK / WS | 7 / 15 / 16 |
| UART TX / RX | 43 / 44 |

The LCD uses SPI mode 3 at 80 MHz, RGB order and colour inversion, matching the
known-working local firmware. Both raw audio buses use 32-bit mono left slots.
The microphone runs at 16 kHz. The known-good speaker clock is 24 kHz, so this
port converts Muse's 16 kHz stream to 24 kHz at the board boundary. Physical
playback and capture still require acceptance on the board.

## Build and flash

From `esp32/`, with ESP-IDF v6.0.1 active:

```sh
tools/muse/board.sh build xiaozhi-154
tools/muse/board.sh flash xiaozhi-154 COM8
```

The COM number can change after reconnecting. This tested board has no separate
reset control: power it off, hold GPIO0/BOOT, power it on, then release BOOT to
enter the ROM loader. Keep the full 16 MB factory backup before the first Muse
flash.

Set the SDK token only in the generated build configuration with
`idf.py -B build-muse-xiaozhi-154 menuconfig`; do not put it in this overlay or
commit it. Muse pairing requires the token to be present in the firmware build.

## First hardware acceptance

1. Confirm a correctly oriented, correctly coloured 240×240 Muse UI.
2. Confirm GPIO39 opens/steps through the two-button menu and GPIO10 selects.
3. Pair from the Muse app and confirm with GPIO10.
4. Hold GPIO10 to record a short voice turn; check microphone level and reply.
5. Check speaker playback for speed, distortion and volume before enabling any
   higher default volume.
