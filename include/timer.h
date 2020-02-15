
/**
 *
 * Copyright (c) 2020, Jared Wolff
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef TIMER_H
#define TIMER_H

#include "app_timer.h"

#define timer_define(name) _timer_define(name)

// Callback
typedef void (*timer_evt_t)();

// Struct for keeping track of timer_id and event handler
typedef struct
{
    const app_timer_id_t *timer_id;
    timer_evt_t timer_evt;
    volatile bool raw_evt_enabled;
} timer_id_t;

// Initializer
#define _timer_define(name)                \
    APP_TIMER_DEF(CONCAT_2(name, _app));   \
    static timer_id_t name = {             \
        .timer_id = &CONCAT_2(name, _app), \
        .timer_evt = NULL,                 \
        .raw_evt_enabled = false,          \
    }

/**@brief Timer modes. */
typedef enum
{
    TIMER_SINGLE_SHOT = APP_TIMER_MODE_SINGLE_SHOT, /**< The timer will expire only once. */
    TIMER_REPEATED = APP_TIMER_MODE_REPEATED        /**< The timer will restart each time it expires. */
} timer_mode_t;

void timer_create(timer_id_t *p_timer_id, timer_mode_t mode,
                  timer_evt_t timeout_handler);

void timer_start(timer_id_t *p_timer_id, uint32_t timeout_ms);

void timer_stop(timer_id_t *p_timer_id);

bool timer_is_active(timer_id_t *p_timer_id);

void timer_reset(timer_id_t *p_timer_id);

void timer_raw_evt_enabled(timer_id_t *p_timer_id, bool enabled);

void timer_process();

#endif