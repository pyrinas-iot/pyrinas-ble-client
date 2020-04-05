/**
 * Copyright (c) 2016 - 2019, Nordic Semiconductor ASA
 * Copyright (c) 2020, Jared Wolff
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#include <stdint.h>
#include <stdio.h>

#include "app_error_weak.h"
#include "app_timer.h"
#include "ble_m.h"
#include "buttons_m.h"
#include "flash.h"
#include "fs.h"
#include "hardfault.h"
#include "pm_m.h"
#include "util.h"

#include "FreeRTOS.h"
#include "task.h"

#include "cellular_port.h"

#include "nrf_clock.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_pwr_mgmt.h"

#include "app.h"

#if NRF_LOG_ENABLED
static TaskHandle_t m_logger_thread; /**< Definition of Logger thread. */
#endif

static TaskHandle_t m_main_thread;

/**@brief Thread for handling main context
 *
 * @details This thread the replacment for the main loop.
 *
 * @param[in]   arg   Pointer used for passing some arbitrary information (context) from the
 *                    osThreadCreate() call to the thread.
 */
static void main_thread(void *arg)
{

    // Startup message
    NRF_LOG_INFO("Pyrinas started.");

    // Init cellular
    cellular_init();

    // App side related
    // setup();

    while (true)
    {
        // Processing in main loop.
        // ble_process();

        // Process serial errors
        // serial_process();

        // Process timers
        // timer_process();

        // App side related
        // loop();

        // // Manage power if there's nothing else to do.
        // if (NRF_LOG_PROCESS() == false)
        // {
        //     nrf_pwr_mgmt_run();
        // }

        vTaskSuspend(NULL); // Suspend myself
    }
}

#if NRF_LOG_ENABLED
/**@brief Thread for handling the logger.
 *
 * @details This thread is responsible for processing log entries if logs are deferred.
 *          Thread flushes all log entries and suspends. It is resumed by idle task hook.
 *
 * @param[in]   arg   Pointer used for passing some arbitrary information (context) from the
 *                    osThreadCreate() call to the thread.
 */
static void logger_thread(void *arg)
{
    UNUSED_PARAMETER(arg);

    while (1)
    {
        NRF_LOG_FLUSH();

        vTaskSuspend(NULL); // Suspend myself
    }
}
#endif //NRF_LOG_ENABLED

/**@brief Function for initializing nrf logger.
 */
static void logs_init()
{
    ret_code_t err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_DEFAULT_BACKENDS_INIT();

#if NRF_LOG_ENABLED
    // Start execution.
    if (pdPASS != xTaskCreate(logger_thread, "LOGGER", 256, NULL, 4, &m_logger_thread))
    {
        APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
    }
#endif
}

/**@brief Function for initializing application timer.
 */
static void timer_init()
{
    // Check if LF clock is running
    if (!nrf_clock_lf_is_running())
    {
        nrf_clock_task_trigger(NRF_CLOCK_TASK_LFCLKSTART);
    }

    // Then init!
    ret_code_t err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);
}

/**@brief A function which is hooked to idle task.
 * @note Idle hook must be enabled in FreeRTOS configuration (configUSE_IDLE_HOOK).
 */
void vApplicationIdleHook(void)
{
#if NRF_LOG_ENABLED
    vTaskResume(m_logger_thread);
#endif

    // Resumes main thread as well.
    vTaskResume(m_main_thread);
}

/**@brief   Function for application main entry.
 *
 * @details Initializes BLE and NFC stacks and runs NFC Forum device task.
 */
int main(void)
{
    logs_init();
    timer_init();
    buttons_init();
    peer_manager_init(false);

    flash_init();
    fs_init();

    // Start execution of main thread.
    if (pdPASS != xTaskCreate(main_thread, "MAIN", 256, NULL, 2, &m_main_thread))
    {
        APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
    }

    NRF_LOG_INFO("Before scheduler.");

    // Start FreeRTOS scheduler.
    // Note: this blocks infinitely..
    vTaskStartScheduler();

    for (;;)
    {
        APP_ERROR_HANDLER(NRF_ERROR_FORBIDDEN);
    }
}

/**@brief Function to handle asserts in the SoftDevice.
 *
 * @details This function will be called in case of an assert in the SoftDevice.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyze
 *          how your product is supposed to react in case of Assert.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in] line_num     Line number of the failing ASSERT call.
 * @param[in] p_file_name  File name of the failing ASSERT call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t *p_file_name)
{
    app_error_handler(0xDEADBEEF, line_num, p_file_name);
}
