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
#include "pm_m.h"
#include "util.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_pwr_mgmt.h"

/**@brief Function for initializing nrf logger.
 */
static void logs_init()
{
    ret_code_t err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_DEFAULT_BACKENDS_INIT();
}

/**@brief Function for initializing application timer.
 */
static void timer_init()
{
    ret_code_t err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);
}

static void back_handler(char *name, char *data)
{
    NRF_LOG_INFO("%s: %s", name, data);
}

/**@brief Function recieving events (central or peripheral)
 */
static void ble_raw_evt_handler(protobuf_event_t *p_evt)
{

    static protobuf_event_t evt;

    evt = *p_evt;

    evt.name.bytes[evt.name.size++] = 0;
    evt.data.bytes[evt.data.size++] = 0;

    NRF_LOG_INFO("%s: %s", evt.name.bytes, evt.data.bytes);
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

// Replace BLE_STACK_PERIPH_DEF with BLE_STACK_CENTRAL_DEF for central mode
#if BOARD_MODE == ble_mode_peripheral
    BLE_STACK_PERIPH_DEF(init);
#else
    BLE_STACK_CENTRAL_DEF(init);

    // TODO: move out of init. Adding a separate function "attaching"
    // TODO: -> new devices.
    // Add an address to scan for
    ble_gap_addr_t dev = {
        .addr_type = BLE_GAP_ADDR_TYPE_RANDOM_STATIC,
        .addr = {0x81, 0x64, 0x4C, 0xAD, 0x7D, 0xC0}};

    // 7c:84:9d:32:8d:e4
    ble_gap_addr_t two = {
        .addr_type = BLE_GAP_ADDR_TYPE_RANDOM_STATIC,
        .addr = {0x7c, 0x84, 0x9d, 0x32, 0x8d, 0xe4}};

    init.config.devices[0] = dev;
    init.config.devices[1] = two;

    // Increment the device_count
    init.config.device_count = 2;

#endif

    // Configuration for ble stack
    ble_stack_init(&init);

    // Subscribe to raw events
    // For more advanced use cases
    ble_subscribe_raw(ble_raw_evt_handler);

    // Peer manager config
    peer_manager_init(false);

    advertising_start();
    scan_start();

    ble_gap_addr_t gap_addr;
    sd_ble_gap_addr_get(&gap_addr);
    char gap_addr_str[18];

    // Convert address to readable string
    bin_to_strhex_delim(gap_addr.addr, BLE_GAP_ADDR_LEN, ':', gap_addr_str);

    // Startup message
    NRF_LOG_INFO("Scaffolding started.");
    NRF_LOG_INFO("Address: %s", gap_addr_str);

    // Publish test
    ble_publish("ping", "");

    // Subscribe test
    ble_subscribe("back", back_handler);

    while (true)
    {
        // Processing in main loop.
        ble_process();

        // Manage power if there's nothing else to do.
        if (NRF_LOG_PROCESS() == false)
        {
            nrf_pwr_mgmt_run();
        }
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
