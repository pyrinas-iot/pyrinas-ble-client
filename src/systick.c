#include "systick.h"

#include "sdk_macros.h"
#include "app_timer.h"
#include "limits.h"

#define NRF_LOG_MODULE_NAME systick
#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();

APP_TIMER_DEF(m_timer);

static systick_ticks_t m_ticks = 0;

static void systick_timer_handler(void *p_context)
{
    m_ticks += 1;
}

// TODO: do this more async without relying on processor/interrupts.
uint32_t systick_init(void)
{
    ret_code_t err_code = app_timer_create(&m_timer, APP_TIMER_MODE_REPEATED, systick_timer_handler);
    VERIFY_SUCCESS(err_code);

    // Then start
    err_code = app_timer_start(m_timer, APP_TIMER_TICKS(1), NULL);
    VERIFY_SUCCESS(err_code);

    return err_code;
}

systick_ticks_t systick_get_ticks(void)
{
    return m_ticks;
}

systick_ticks_t systick_get_diff_now(systick_ticks_t begin)
{
    // Check if we've overflowed
    if (m_ticks < begin)
    {
        // Get the diff from begin to overflow
        uint32_t fdiff = UINT_MAX - begin;

        // Then add m_ticks and return sum
        return fdiff + m_ticks;
    }
    else
    {
        // Return diff
        return m_ticks - begin;
    }
}