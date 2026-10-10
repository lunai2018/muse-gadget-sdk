/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Custom Xiaozhi ESP32-S3-N16R8 board: 1.54" 240x240 ST7789 (no touch),
 * two user buttons, and separate raw-I2S microphone and speaker buses.
 *
 * The LCD and button map comes from this board's previously validated Buddy
 * firmware (xiaozhi-buddy/main/display/tft.cc and input/button.cc). The audio
 * map, sample rates, and 32-bit mono format come from 78/xiaozhi-esp32's
 * main/boards/zhengchen/1.54tft-wifi and NoAudioCodecSimplex implementation.
 * GPIO0 stays reserved as the ROM download button.
 */
#include <math.h>
#include <string.h>

#include "driver/i2s_std.h"
#include "driver/ledc.h"
#include "driver/spi_master.h"
#include "esp_check.h"
#include "esp_codec_dev.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_st7789.h"
#include "esp_log.h"
#include "esp_lv_adapter.h"
#include "esp_sleep.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "muse_audio.h"
#include "muse_board.h"
#include "muse_mem.h"

static const char *TAG = "board";

#define LCD_W 240
#define LCD_H 240
#define LCD_HOST SPI3_HOST
#define LCD_MOSI GPIO_NUM_41
#define LCD_SCLK GPIO_NUM_42
#define LCD_DC GPIO_NUM_40
#define LCD_RST GPIO_NUM_45
#define LCD_CS GPIO_NUM_21
#define LCD_BL GPIO_NUM_20
#define DRAW_BUF_LINES 20

#define TALK_GPIO GPIO_NUM_10
#define AUX_GPIO GPIO_NUM_39
/* The Xiaozhi volume-up switch can produce a second, fully debounced pulse
 * just after release (observed 61 ms later on the physical board).  Treat a
 * re-press in this window, and its matching release, as contact bounce. */
#define TALK_REPRESS_GUARD_MS 150

#define MIC_WS GPIO_NUM_4
#define MIC_SCLK GPIO_NUM_5
#define MIC_DIN GPIO_NUM_6
#define SPK_DOUT GPIO_NUM_7
#define SPK_BCLK GPIO_NUM_15
#define SPK_WS GPIO_NUM_16
#define SPK_RATE 24000

/* The raw-mic reference firmware applies no software gain. Muse presents
 * 30 dB as its normal default, so make that setting unity for this board. */
#define MIC_GAIN_OFFSET_DB 30

static esp_lcd_panel_handle_t s_panel;
static muse_gpio_button_t s_talk, s_aux;
static TickType_t s_talk_release_tick;
static bool s_talk_repress_ignored;
static i2s_chan_handle_t s_tx, s_rx;
static bool s_tx_on, s_rx_on;
static int s_mic_gain_q8 = 256;
static int32_t s_spk_buf[MUSE_AUDIO_CHUNK * 3 / 2];
static int32_t s_mic_buf[MUSE_AUDIO_CHUNK];
static int32_t s_mic_dc_q8;

static esp_err_t init(void)
{
    ESP_RETURN_ON_ERROR(muse_gpio_button_init(&s_talk, TALK_GPIO), TAG, "talk button");
    ESP_RETURN_ON_ERROR(muse_gpio_button_init(&s_aux, AUX_GPIO), TAG, "aux button");
    return ESP_OK;
}

static lv_display_t *display_start(lv_indev_t **touch)
{
    (void)touch;
    const ledc_timer_config_t bl_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = 20000,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    const ledc_channel_config_t bl_ch = {
        .gpio_num = LCD_BL,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
    };
    if (ledc_timer_config(&bl_timer) != ESP_OK || ledc_channel_config(&bl_ch) != ESP_OK) {
        return NULL;
    }

    const spi_bus_config_t bus = {
        .sclk_io_num = LCD_SCLK,
        .mosi_io_num = LCD_MOSI,
        .miso_io_num = GPIO_NUM_NC,
        .quadwp_io_num = GPIO_NUM_NC,
        .quadhd_io_num = GPIO_NUM_NC,
        .max_transfer_sz = LCD_W * DRAW_BUF_LINES * 2,
    };
    if (spi_bus_initialize(LCD_HOST, &bus, SPI_DMA_CH_AUTO) != ESP_OK) {
        return NULL;
    }
    esp_lcd_panel_io_handle_t io;
    const esp_lcd_panel_io_spi_config_t io_cfg = {
        .cs_gpio_num = LCD_CS,
        .dc_gpio_num = LCD_DC,
        .spi_mode = 3,
        .pclk_hz = 80 * 1000 * 1000,
        .trans_queue_depth = 10,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
    };
    if (esp_lcd_new_panel_io_spi(LCD_HOST, &io_cfg, &io) != ESP_OK) {
        return NULL;
    }
    const esp_lcd_panel_dev_config_t panel_cfg = {
        .reset_gpio_num = LCD_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
    };
    if (esp_lcd_new_panel_st7789(io, &panel_cfg, &s_panel) != ESP_OK) {
        return NULL;
    }
    ESP_ERROR_CHECK(esp_lcd_panel_reset(s_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(s_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(s_panel, false));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(s_panel, false, false));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(s_panel, true));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(s_panel, true));

    esp_lv_adapter_config_t adapter_cfg = ESP_LV_ADAPTER_DEFAULT_CONFIG();
    adapter_cfg.task_core_id = MUSE_UI_CORE;
    adapter_cfg.task_priority = MUSE_UI_PRIORITY;
    if (esp_lv_adapter_init(&adapter_cfg) != ESP_OK) {
        return NULL;
    }
    const esp_lv_adapter_display_config_t disp_cfg = {
        .panel = s_panel,
        .panel_io = io,
        .profile = {
            .interface = ESP_LV_ADAPTER_PANEL_IF_OTHER,
            .rotation = ESP_LV_ADAPTER_ROTATE_0,
            .hor_res = LCD_W,
            .ver_res = LCD_H,
            .buffer_height = DRAW_BUF_LINES,
            .use_psram = false,
            .require_double_buffer = true,
        },
        .tear_avoid_mode = ESP_LV_ADAPTER_TEAR_AVOID_MODE_NONE,
    };
    lv_display_t *disp = esp_lv_adapter_register_display(&disp_cfg);
    if (!disp || esp_lv_adapter_start() != ESP_OK) {
        return NULL;
    }
    return disp;
}

static bool display_lock(int timeout_ms)
{
    return esp_lv_adapter_lock(timeout_ms) == ESP_OK;
}

static void set_brightness(int pct)
{
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, pct * 1023 / 100);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}

static void panel_sleep(bool sleep)
{
    esp_lcd_panel_disp_sleep(s_panel, sleep);
}

static void display_pause(bool pause)
{
    if (pause) {
        esp_lv_adapter_pause(-1);
    } else {
        esp_lv_adapter_resume();
    }
}

/* The board has no codec. Muse opens stereo 16-bit streams; the hardware is
 * mono, 32-bit, left-slot I2S. Convert at the esp_codec_dev data boundary. */
static int audio_enable(const audio_codec_data_if_t *h, esp_codec_dev_type_t type, bool on)
{
    (void)h;
    i2s_chan_handle_t chan = type == ESP_CODEC_DEV_TYPE_IN ? s_rx : s_tx;
    bool *enabled = type == ESP_CODEC_DEV_TYPE_IN ? &s_rx_on : &s_tx_on;
    if (on == *enabled) {
        return ESP_CODEC_DEV_OK;
    }
    esp_err_t err = on ? i2s_channel_enable(chan) : i2s_channel_disable(chan);
    if (err != ESP_OK) {
        return ESP_CODEC_DEV_DRV_ERR;
    }
    *enabled = on;
    return ESP_CODEC_DEV_OK;
}

static int spk_write(const audio_codec_data_if_t *h, uint8_t *data, int size)
{
    (void)h;
    const int16_t *in = (const int16_t *)data;
    int frames = size / (2 * (int)sizeof(int16_t));
    for (int done = 0; done < frames;) {
        int n = frames - done > MUSE_AUDIO_CHUNK ? MUSE_AUDIO_CHUNK : frames - done;
        int out_n = 0;
        for (int i = 0; i + 1 < n; i += 2) {
            int32_t a = ((int32_t)in[2 * (done + i)] + in[2 * (done + i) + 1]) / 2;
            int32_t b = ((int32_t)in[2 * (done + i + 1)] + in[2 * (done + i + 1) + 1]) / 2;
            /* The cloud stream is 16 kHz and this board's known-good speaker
             * clock is 24 kHz. Two source samples become three output samples. */
            s_spk_buf[out_n++] = a * 65536;
            s_spk_buf[out_n++] = ((a + b) / 2) * 65536;
            s_spk_buf[out_n++] = b * 65536;
        }
        if (n & 1) {
            int i = n - 1;
            int32_t mono = ((int32_t)in[2 * (done + i)] + in[2 * (done + i) + 1]) / 2;
            s_spk_buf[out_n++] = mono * 65536;
        }
        size_t wrote = 0;
        if (i2s_channel_write(s_tx, s_spk_buf, out_n * sizeof(s_spk_buf[0]), &wrote, portMAX_DELAY) != ESP_OK ||
            wrote != (size_t)out_n * sizeof(s_spk_buf[0])) {
            return ESP_CODEC_DEV_WRITE_FAIL;
        }
        done += n;
    }
    return ESP_CODEC_DEV_OK;
}

static int mic_read(const audio_codec_data_if_t *h, uint8_t *data, int size)
{
    (void)h;
    int16_t *out = (int16_t *)data;
    int frames = size / (2 * (int)sizeof(int16_t));
    for (int done = 0; done < frames;) {
        int n = frames - done > MUSE_AUDIO_CHUNK ? MUSE_AUDIO_CHUNK : frames - done;
        size_t got = 0;
        if (i2s_channel_read(s_rx, s_mic_buf, n * sizeof(s_mic_buf[0]), &got, pdMS_TO_TICKS(1000)) != ESP_OK ||
            got != (size_t)n * sizeof(s_mic_buf[0])) {
            return ESP_CODEC_DEV_READ_FAIL;
        }
        for (int i = 0; i < n; i++) {
            int32_t raw = s_mic_buf[i] >> 12;
            /* Track the MEMS mic's zero level before applying gain. Otherwise
             * even a modest DC offset becomes clipping at Muse's default. */
            s_mic_dc_q8 += (raw * 256 - s_mic_dc_q8) >> 8;
            int32_t sample = (raw - (s_mic_dc_q8 >> 8)) * s_mic_gain_q8 >> 8;
            int16_t v = sample > INT16_MAX ? INT16_MAX : sample < INT16_MIN ? INT16_MIN : (int16_t)sample;
            out[2 * (done + i)] = v;
            out[2 * (done + i) + 1] = v;
        }
        done += n;
    }
    return ESP_CODEC_DEV_OK;
}

static void set_mic_gain(esp_codec_dev_handle_t mic, int db)
{
    (void)mic;
    s_mic_gain_q8 = (int)(256.0f * powf(10.0f, (db - MIC_GAIN_OFFSET_DB) / 20.0f));
}

static esp_err_t audio_init(esp_codec_dev_handle_t *spk, esp_codec_dev_handle_t *mic)
{
    i2s_chan_config_t tx_chan = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    tx_chan.auto_clear = true;
    ESP_RETURN_ON_ERROR(i2s_new_channel(&tx_chan, &s_tx, NULL), TAG, "speaker i2s channel");
    i2s_std_config_t tx_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(SPK_RATE),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = SPK_BCLK,
            .ws = SPK_WS,
            .dout = SPK_DOUT,
            .din = I2S_GPIO_UNUSED,
        },
    };
    tx_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_LEFT;
    tx_cfg.slot_cfg.ws_width = 32;
    ESP_RETURN_ON_ERROR(i2s_channel_init_std_mode(s_tx, &tx_cfg), TAG, "speaker i2s");

    i2s_chan_config_t rx_chan = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_1, I2S_ROLE_MASTER);
    ESP_RETURN_ON_ERROR(i2s_new_channel(&rx_chan, NULL, &s_rx), TAG, "microphone i2s channel");
    i2s_std_config_t rx_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(MUSE_AUDIO_RATE),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = MIC_SCLK,
            .ws = MIC_WS,
            .dout = I2S_GPIO_UNUSED,
            .din = MIC_DIN,
        },
    };
    rx_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_LEFT;
    rx_cfg.slot_cfg.ws_width = 32;
    ESP_RETURN_ON_ERROR(i2s_channel_init_std_mode(s_rx, &rx_cfg), TAG, "microphone i2s");

    static const audio_codec_data_if_t spk_if = { .enable = audio_enable, .write = spk_write };
    static const audio_codec_data_if_t mic_if = { .enable = audio_enable, .read = mic_read };
    esp_codec_dev_cfg_t out_cfg = { .dev_type = ESP_CODEC_DEV_TYPE_OUT, .data_if = &spk_if };
    esp_codec_dev_cfg_t in_cfg = { .dev_type = ESP_CODEC_DEV_TYPE_IN, .data_if = &mic_if };
    *spk = esp_codec_dev_new(&out_cfg);
    *mic = esp_codec_dev_new(&in_cfg);
    return *spk && *mic ? ESP_OK : ESP_FAIL;
}

static unsigned poll_buttons(void)
{
    unsigned ev = muse_gpio_button_poll(&s_talk)
                | muse_gpio_button_poll(&s_aux) << 2;
    TickType_t now = xTaskGetTickCount();

    if (ev & MUSE_BTN_TALK_PRESS) {
        if (s_talk_release_tick != 0
            && now - s_talk_release_tick < pdMS_TO_TICKS(TALK_REPRESS_GUARD_MS)) {
            ev &= ~MUSE_BTN_TALK_PRESS;
            s_talk_repress_ignored = true;
            ESP_LOGW(TAG, "ignored talk-button re-press after release");
        }
    }
    if (ev & MUSE_BTN_TALK_RELEASE) {
        if (s_talk_repress_ignored) {
            ev &= ~MUSE_BTN_TALK_RELEASE;
            s_talk_repress_ignored = false;
        } else {
            s_talk_release_tick = now;
        }
    }
    return ev;
}

static void wait_buttons(int timeout_ms)
{
    muse_gpio_buttons_wait((muse_gpio_button_t *const[]){ &s_talk, &s_aux }, 2, timeout_ms);
}

/* With no power latch, "off" is deep sleep. GPIO10 is RTC-capable and wakes
 * the board; GPIO39 is not, and GPIO0 remains available for ROM download. */
static esp_err_t power_off(void)
{
    set_brightness(0);
    esp_lcd_panel_disp_on_off(s_panel, false);
    while (gpio_get_level(TALK_GPIO) == 0) {
        vTaskDelay(pdMS_TO_TICKS(20));
    }
    ESP_RETURN_ON_ERROR(esp_sleep_enable_ext0_wakeup(TALK_GPIO, 0), TAG, "wake button");
    esp_deep_sleep_start();
    return ESP_FAIL;
}

static const muse_board_t s_board = {
    .name = "Xiaozhi 1.54",
    .width = LCD_W,
    .height = LCD_H,
    .round = false,
    .touch = false,
    .diagonal_in = 1.54f,
    .talk_button = "volume up",
    .aux_button = "volume down",
    .talk_hint = { LV_ALIGN_RIGHT_MID, -5, -28 },
    .aux_hint = { LV_ALIGN_RIGHT_MID, -5, 28 },
    .frame_ms = 40,
    .init = init,
    .display_start = display_start,
    .display_lock = display_lock,
    .display_unlock = esp_lv_adapter_unlock,
    .set_brightness = set_brightness,
    .panel_sleep = panel_sleep,
    .display_pause = display_pause,
    .audio_init = audio_init,
    .mic_slot = 0,
    .set_mic_gain = set_mic_gain,
    .poll_buttons = poll_buttons,
    .wait_buttons = wait_buttons,
    .power_off = power_off,
};

const muse_board_t *muse_board_get(void)
{
    return &s_board;
}
