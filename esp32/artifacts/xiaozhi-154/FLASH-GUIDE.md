# Xiaozhi 1.54 Muse Gadget v23 flashing guide

Validated target: ESP32-S3-N16R8, 16 MB flash, 8 MB octal PSRAM, 1.54-inch
240 x 240 ST7789, raw-I2S microphone and speaker.

Validated image:

`muse-xiaozhi-154-audio-v23-approval-sleep-wake-app.bin`

SHA-256:

`22A17745F8E5AB1BCCF438622CD267291BC9E961A3ACA961E8B29A2285943FA4`

## Espressif Flash Download Tool

Use the validated application-only update so that saved Wi-Fi, pairing and
device settings remain intact:

| Setting | Value |
|---|---|
| Chip | ESP32-S3 |
| Image address | `0x20000` |
| SPI mode | DIO |
| SPI speed | 80 MHz |
| Flash size | 16 MB |
| Erase | Off |
| DoNotChgBin | On |

Only select the v23 application image row. Do not add bootloader, partition or
OTA-data rows for this update.

This board has no separate reset control. To enter the ROM downloader: switch
power off, hold GPIO0/BOOT, switch power on, then release GPIO0/BOOT. After the
download completes, power-cycle the board normally. Close serial monitors and
other programs that may hold the COM port before flashing.

The validated Windows test used COM8, but the assigned COM number can change
after reconnecting. Select the port currently shown by Device Manager or the
flashing tool.

## Security and provisioning

The repository and binary package do not contain a plaintext Azure key. Azure
Speech credentials are provisioned locally through the connector/setup path
and stored in device NVS. The Muse developer token is also local provisioning
data and must not be committed.

Application-only flashing preserves NVS. A full-chip erase removes Wi-Fi,
pairing, Azure TTS settings and other local credentials, so only erase after
confirming that the factory backup is safe and reprovisioning is intended.

## Expected smoke test

1. Boot reaches `WIFI READY` and automatically reconnects to Muse.
2. TALK records a voice note; release sends exactly once.
3. The reply appears full-screen and Azure TTS plays through the speaker.
4. During playback, TALK stops playback and immediately starts a new recording.
5. VOLUME short-press changes text pages; long-press exits to the avatar.
6. `approval.request` wakes a black display and supports Approve, Deny and
   Timeout before returning to the avatar.
