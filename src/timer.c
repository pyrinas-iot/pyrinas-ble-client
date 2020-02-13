#include "timer.h"

#include "app_error.h"

timer_evt_t m_evt_handler = NULL;

static void app_timer_timeout_handler(void *p_context)
{
    if (m_evt_handler != NULL) m_evt_handler();
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