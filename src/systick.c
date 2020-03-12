#include "systick.h"

#include "FreeRTOS.h"
#include "limits.h"
#include "task.h"

#define NRF_LOG_MODULE_NAME systick
#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();

systick_ticks_t systick_get_ticks(void)
{
    return (systick_ticks_t)xTaskGetTickCount();
}

systick_ticks_t systick_get_diff_now(systick_ticks_t begin)
{
    systick_ticks_t now = (systick_ticks_t)xTaskGetTickCount();

    // Check if we've overflowed
    if (now < begin)
    {
        // Get the diff from begin to overflow
        uint32_t fdiff = UINT_MAX - begin;

        // Then add m_ticks and return sum
        return fdiff + now;
    }
    else
    {
        // Return diff
        return now - begin;
    }

    return 0;
}