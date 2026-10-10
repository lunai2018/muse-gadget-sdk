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

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MUSE_APPROVAL_PROMPT_MAX 384
#define MUSE_APPROVAL_DETAIL_MAX 256

typedef enum {
    MUSE_APPROVAL_APPROVED,
    MUSE_APPROVAL_DENIED,
    MUSE_APPROVAL_TIMED_OUT,
    MUSE_APPROVAL_NOT_DISPLAYED,
    MUSE_APPROVAL_CANCELLED,
} muse_approval_result_t;

/* One confirmation card may be active at a time. */
void muse_approval_init(void);
bool muse_approval_begin(const char *prompt, const char *detail);
bool muse_approval_active(void);

/* Copies a coherent UI snapshot. version changes whenever the card changes. */
bool muse_approval_snapshot(char *prompt, size_t prompt_cap,
                            char *detail, size_t detail_cap,
                            uint32_t *version);

/* The UI calls this after making the card visible. Approval buttons and the
 * user timeout are deliberately disabled until that acknowledgement. */
void muse_approval_mark_visible(uint32_t version);
bool muse_approval_visible(void);

/* Called by the input task. Returns false if no card was pending. */
bool muse_approval_respond(bool approved);

/* Called by the command worker after begin(). The display wait is separate so
 * the requested decision timeout starts only after the card is visible. */
bool muse_approval_wait_visible(TickType_t timeout);
muse_approval_result_t muse_approval_wait(TickType_t timeout);
void muse_approval_cancel(void);

#ifdef __cplusplus
}
#endif
