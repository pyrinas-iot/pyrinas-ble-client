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

timer_evt_t m_evt_handler = NULL;

static void app_timer_timeout_handler(void *p_context)
{
    if (m_evt_handler != NULL)
        m_evt_handler();
}

void timer_create(const app_timer_id_t *p_timer_id, timer_mode_t mode,
                  timer_evt_t timeout_handler)
{

    if (timeout_handler != NULL)
    {
        m_evt_handler = timeout_handler;
    }

    ret_code_t err_code = app_timer_create(p_timer_id, mode, app_timer_timeout_handler);
    APP_ERROR_CHECK(err_code);
}

void timer_start(const app_timer_id_t *p_timer_id, uint32_t timeout_ms)
{

    ret_code_t err_code = app_timer_start(*p_timer_id, APP_TIMER_TICKS(timeout_ms), NULL);
    APP_ERROR_CHECK(err_code);
}

void timer_stop(const app_timer_id_t *p_timer_id)
{
    ret_code_t err_code = app_timer_stop(*p_timer_id);
    APP_ERROR_CHECK(err_code);
}