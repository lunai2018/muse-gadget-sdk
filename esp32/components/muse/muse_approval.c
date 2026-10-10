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

#include "muse_approval.h"

#include <string.h>

#include "freertos/event_groups.h"

#define APPROVED_BIT BIT0
#define DENIED_BIT   BIT1
#define CANCELLED_BIT BIT2
#define VISIBLE_BIT BIT3
#define RESULT_BITS (APPROVED_BIT | DENIED_BIT | CANCELLED_BIT)

static EventGroupHandle_t s_events;
static portMUX_TYPE s_lock = portMUX_INITIALIZER_UNLOCKED;
static bool s_starting;
static bool s_active;
static bool s_visible;
static uint32_t s_version;
static char s_prompt[MUSE_APPROVAL_PROMPT_MAX + 1];
static char s_detail[MUSE_APPROVAL_DETAIL_MAX + 1];

void muse_approval_init(void)
{
    if (!s_events) s_events = xEventGroupCreate();
}

bool muse_approval_begin(const char *prompt, const char *detail)
{
    if (!s_events || !prompt || !prompt[0]) return false;
    portENTER_CRITICAL(&s_lock);
    if (s_active || s_starting) {
        portEXIT_CRITICAL(&s_lock);
        return false;
    }
    /* Reserve the only card before clearing the previous result. Input cannot
     * answer until active becomes true, so an immediate press is never lost. */
    s_starting = true;
    portEXIT_CRITICAL(&s_lock);
    xEventGroupClearBits(s_events, RESULT_BITS | VISIBLE_BIT);

    portENTER_CRITICAL(&s_lock);
    strlcpy(s_prompt, prompt, sizeof(s_prompt));
    strlcpy(s_detail, detail ? detail : "", sizeof(s_detail));
    s_active = true;
    s_visible = false;
    s_starting = false;
    s_version++;
    portEXIT_CRITICAL(&s_lock);
    return true;
}

void muse_approval_mark_visible(uint32_t version)
{
    portENTER_CRITICAL(&s_lock);
    bool mark = s_active && s_version == version && !s_visible;
    if (mark) s_visible = true;
    portEXIT_CRITICAL(&s_lock);
    if (mark) xEventGroupSetBits(s_events, VISIBLE_BIT);
}

bool muse_approval_visible(void)
{
    portENTER_CRITICAL(&s_lock);
    bool visible = s_active && s_visible;
    portEXIT_CRITICAL(&s_lock);
    return visible;
}

bool muse_approval_active(void)
{
    portENTER_CRITICAL(&s_lock);
    bool active = s_active;
    portEXIT_CRITICAL(&s_lock);
    return active;
}

bool muse_approval_snapshot(char *prompt, size_t prompt_cap,
                            char *detail, size_t detail_cap,
                            uint32_t *version)
{
    portENTER_CRITICAL(&s_lock);
    bool active = s_active;
    if (prompt && prompt_cap) strlcpy(prompt, s_prompt, prompt_cap);
    if (detail && detail_cap) strlcpy(detail, s_detail, detail_cap);
    if (version) *version = s_version;
    portEXIT_CRITICAL(&s_lock);
    return active;
}

static bool finish(EventBits_t bit)
{
    portENTER_CRITICAL(&s_lock);
    bool active = s_active;
    if (active) {
        s_active = false;
        s_visible = false;
        s_version++;
    }
    portEXIT_CRITICAL(&s_lock);
    if (active) xEventGroupSetBits(s_events, bit);
    return active;
}

bool muse_approval_respond(bool approved)
{
    if (!muse_approval_visible()) return false;
    return finish(approved ? APPROVED_BIT : DENIED_BIT);
}

bool muse_approval_wait_visible(TickType_t timeout)
{
    EventBits_t bits = xEventGroupWaitBits(s_events, VISIBLE_BIT, pdFALSE, pdFALSE, timeout);
    return (bits & VISIBLE_BIT) != 0;
}

muse_approval_result_t muse_approval_wait(TickType_t timeout)
{
    EventBits_t bits = xEventGroupWaitBits(s_events, RESULT_BITS, pdTRUE, pdFALSE, timeout);
    if (bits & APPROVED_BIT) return MUSE_APPROVAL_APPROVED;
    if (bits & DENIED_BIT) return MUSE_APPROVAL_DENIED;
    if (bits & CANCELLED_BIT) return MUSE_APPROVAL_CANCELLED;
    finish(DENIED_BIT);
    xEventGroupClearBits(s_events, RESULT_BITS);
    return MUSE_APPROVAL_TIMED_OUT;
}

void muse_approval_cancel(void)
{
    finish(CANCELLED_BIT);
}
