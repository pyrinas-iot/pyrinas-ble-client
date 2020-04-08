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

#include "timer.h"

#include "app_error.h"

#include "nrf_log.h"
#include "nrf_queue.h"

NRF_QUEUE_DEF(timer_id_t, m_timer_event_queue, 8, NRF_QUEUE_MODE_OVERFLOW);

static void app_timer_timeout_handler(void *p_context)
{
    // Convert to callback
    timer_id_t *timer_id = (timer_id_t *)p_context;

    // Depending on how it's configured, this will
    // queue the event handler or run it right now
    if (timer_id->raw_evt_enabled)
    {
        timer_id->timer_evt();
    }
    else
    {
        ret_code_t ret = nrf_queue_push(&m_timer_event_queue, timer_id);
        APP_ERROR_CHECK(ret);
    }

    // TODO: allow "raw" access?
}

void timer_create(timer_id_t *p_timer_id, timer_mode_t mode,
                  timer_evt_t timeout_handler)
{

    // Set timeout handler
    if (timeout_handler != NULL)
    {
        p_timer_id->timer_evt = timeout_handler;
    }

    ret_code_t err_code = app_timer_create(p_timer_id->timer_id, mode, app_timer_timeout_handler);
    APP_ERROR_CHECK(err_code);
}

void timer_start(timer_id_t *p_timer_id, uint32_t timeout_ms)
{
    // Update the ms value
    p_timer_id->timeout = timeout_ms;

    // Start the timer
    ret_code_t err_code = app_timer_start(*p_timer_id->timer_id, APP_TIMER_TICKS(p_timer_id->timeout), (void *)p_timer_id);
    APP_ERROR_CHECK(err_code);
}

void timer_stop(timer_id_t *p_timer_id)
{
    ret_code_t err_code = app_timer_stop(*p_timer_id->timer_id);
    APP_ERROR_CHECK(err_code);
}

bool timer_is_active(timer_id_t *p_timer_id)
{
    return false; // TODO: fix this!
}

void timer_process()
{
    // Dequeue one item if not empty
    if (!nrf_queue_is_empty(&m_timer_event_queue))
    {

        // Grab the event handler and run it in main context
        timer_id_t timer_id;
        ret_code_t ret = nrf_queue_pop(&m_timer_event_queue, &timer_id);
        APP_ERROR_CHECK(ret);

        // Execute the evt if for realises
        if (timer_id.timer_evt != NULL)
        {
            timer_id.timer_evt();
        }
    }
}

void timer_raw_evt_enabled(timer_id_t *p_timer_id, bool enabled)
{
    p_timer_id->raw_evt_enabled = enabled;
}

// Uses the previous timeout and restarts
void timer_restart(timer_id_t *p_timer_id)
{
    timer_stop(p_timer_id);
    timer_start(p_timer_id, p_timer_id->timeout);
}