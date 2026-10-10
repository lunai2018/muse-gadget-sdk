/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef esp_err_t (*muse_azure_tts_chunk_cb_t)(const uint8_t *data, size_t len, void *ctx);

/* 把文字提交到 Azure Speech，并在 HTTPS 响应到达时逐块交给播放器。 */
esp_err_t muse_azure_tts_stream(const char *text, muse_azure_tts_chunk_cb_t chunk_cb,
                               void *ctx, size_t *total_bytes);

#ifdef __cplusplus
}
#endif
