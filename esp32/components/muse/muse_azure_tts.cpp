/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 */

#include "muse_azure_tts.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "esp_crt_bundle.h"
#include "esp_http_client.h"
#include "esp_log.h"

#include "muse_settings.h"

static const char *TAG = "muse_azure_tts";

struct receive_t {
    muse_azure_tts_chunk_cb_t chunk_cb;
    void *ctx;
    size_t total;
    esp_err_t callback_err;
};

static esp_err_t on_http_event(esp_http_client_event_t *event)
{
    if (event->event_id != HTTP_EVENT_ON_DATA || event->data_len <= 0) {
        return ESP_OK;
    }
    receive_t *rx = static_cast<receive_t *>(event->user_data);
    if (!rx || !rx->chunk_cb) {
        return ESP_ERR_INVALID_STATE;
    }
    size_t n = static_cast<size_t>(event->data_len);
    rx->callback_err = rx->chunk_cb(static_cast<const uint8_t *>(event->data), n, rx->ctx);
    if (rx->callback_err == ESP_OK) {
        rx->total += n;
    }
    return rx->callback_err;
}

/* Azure 的 SSML 正文必须转义，回复文字不能直接插入 XML。 */
static char *xml_escape(const char *text)
{
    size_t need = 1;
    for (const char *p = text; *p; p++) {
        switch (*p) {
        case '&': need += 5; break;
        case '<': case '>': need += 4; break;
        case '\'': case '"': need += 6; break;
        default: need++; break;
        }
    }
    char *out = static_cast<char *>(malloc(need));
    if (!out) {
        return nullptr;
    }
    char *q = out;
    for (const char *p = text; *p; p++) {
        const char *escaped = nullptr;
        switch (*p) {
        case '&': escaped = "&amp;"; break;
        case '<': escaped = "&lt;"; break;
        case '>': escaped = "&gt;"; break;
        case '\'': escaped = "&apos;"; break;
        case '"': escaped = "&quot;"; break;
        default: *q++ = *p; break;
        }
        if (escaped) {
            size_t n = strlen(escaped);
            memcpy(q, escaped, n);
            q += n;
        }
    }
    *q = '\0';
    return out;
}

esp_err_t muse_azure_tts_stream(const char *text, muse_azure_tts_chunk_cb_t chunk_cb,
                               void *ctx, size_t *total_bytes)
{
    if (!text || !text[0] || !chunk_cb || !total_bytes) {
        return ESP_ERR_INVALID_ARG;
    }
    *total_bytes = 0;

    char key[MUSE_TTS_KEY_MAX + 1];
    char region[MUSE_TTS_REGION_MAX + 1];
    char voice[MUSE_TTS_VOICE_MAX + 1];
    muse_settings_azure_tts_key(key);
    muse_settings_azure_tts_region(region);
    muse_settings_azure_tts_voice(voice);
    if (!key[0] || !region[0] || !voice[0]) {
        return ESP_ERR_INVALID_STATE;
    }

    char *escaped = xml_escape(text);
    if (!escaped) {
        memset(key, 0, sizeof(key));
        return ESP_ERR_NO_MEM;
    }
    size_t body_cap = strlen(escaped) + strlen(voice) + 128;
    char *body = static_cast<char *>(malloc(body_cap));
    if (!body) {
        free(escaped);
        memset(key, 0, sizeof(key));
        return ESP_ERR_NO_MEM;
    }
    snprintf(body, body_cap,
             "<speak version='1.0' xml:lang='en-US'><voice name='%s'>%s</voice></speak>",
             voice, escaped);
    free(escaped);

    char url[160];
    snprintf(url, sizeof(url), "https://%s.tts.speech.microsoft.com/cognitiveservices/v1", region);
    receive_t rx = { .chunk_cb = chunk_cb, .ctx = ctx, .total = 0, .callback_err = ESP_OK };
    esp_http_client_config_t config = {};
    config.url = url;
    config.method = HTTP_METHOD_POST;
    config.event_handler = on_http_event;
    config.user_data = &rx;
    config.timeout_ms = 30000;
    config.buffer_size = 4096;
    config.buffer_size_tx = 4096;
    config.crt_bundle_attach = esp_crt_bundle_attach;

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        free(body);
        memset(key, 0, sizeof(key));
        return ESP_ERR_NO_MEM;
    }
    esp_http_client_set_header(client, "Ocp-Apim-Subscription-Key", key);
    esp_http_client_set_header(client, "Content-Type", "application/ssml+xml");
    /* 32 kbps is ample for mono speech and gives the 512 KiB rolling buffer
     * roughly two minutes of headroom instead of about thirty seconds. */
    esp_http_client_set_header(client, "X-Microsoft-OutputFormat", "audio-16khz-32kbitrate-mono-mp3");
    esp_http_client_set_header(client, "User-Agent", "MuseGadget-Xiaozhi");
    esp_http_client_set_post_field(client, body, static_cast<int>(strlen(body)));

    esp_err_t err = esp_http_client_perform(client);
    int status = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);
    free(body);
    memset(key, 0, sizeof(key));

    if (err != ESP_OK || status != 200 || rx.callback_err != ESP_OK || !rx.total) {
        ESP_LOGE(TAG, "Azure TTS failed: transport=%s HTTP=%d callback=%s bytes=%u",
                 esp_err_to_name(err), status, esp_err_to_name(rx.callback_err),
                 static_cast<unsigned>(rx.total));
        return rx.callback_err != ESP_OK ? rx.callback_err : err != ESP_OK ? err : ESP_FAIL;
    }
    *total_bytes = rx.total;
    ESP_LOGI(TAG, "Azure TTS streamed %u MP3 bytes", static_cast<unsigned>(rx.total));
    return ESP_OK;
}
