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
#include "util.h"

#include "ble_central.h"
#include "ble_m.h"
#include "ble_peripheral.h"
#include "fds.h"
#include "nordic_common.h"
#include "nrf_ble_gatt.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"

#include "pb_decode.h"
#include "pb_encode.h"

#define NRF_LOG_MODULE_NAME ble_m
#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();

#define APP_BLE_CONN_CFG_TAG 1  /**< Tag for the configuration of the BLE stack. */
#define APP_BLE_OBSERVER_PRIO 3 /**< BLE observer priority of the application. There is no need to modify this value. */

// TODO: connect to inner portions of cthe code
static bool m_is_connected = false; /**< Flag to keep track of the BLE connections with peripheral devices. */

// TODO: connect to inner portions of cthe code
static uint16_t m_conn_handle = BLE_CONN_HANDLE_INVALID; /**< Current connection handle. */

NRF_BLE_GATT_DEF(m_gatt); /**< GATT module instance. */

static ble_subscription_list_t m_subscribe_list; /**< Use for adding/removing subscriptions */

static ble_stack_init_t m_config; /**< Init config */

static raw_susbcribe_handler_t m_raw_handler_ext;

bool ble_is_connected(void)
{
    return m_is_connected;
}

uint16_t ble_get_conn_handle(void)
{
    return m_conn_handle;
}

void ble_disconnect(void)
{
    ret_code_t err_code;

    if (m_is_connected)
    {
        err_code = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
        if (err_code != NRF_ERROR_INVALID_STATE)
        {
            APP_ERROR_CHECK(err_code);
        }
    }
}

void ble_publish(char *name, char *data)
{

    uint8_t name_length = strlen(name);
    uint8_t data_length = strlen(data);

    // Check size
    if (name_length > member_size(protobuf_event_t_name_t, bytes))
    {
        NRF_LOG_WARNING("Name must be <= %d characters.", member_size(protobuf_event_t_name_t, bytes));
        return;
    }

    // Check size
    if (data_length > member_size(protobuf_event_t_data_t, bytes))
    {
        NRF_LOG_WARNING("Data must be <= %d characters.", member_size(protobuf_event_t_data_t, bytes));
        return;
    }

    // Create an event.
    protobuf_event_t event = {
        .name.size = name_length,
        .data.size = data_length,
    };

    // Copy contentsof message over
    memcpy(event.name.bytes, name, member_size(protobuf_event_t_name_t, bytes));
    memcpy(event.data.bytes, data, member_size(protobuf_event_t_data_t, bytes));

    // Then publish it as a raw format.
    ble_publish_raw(event);
}

void ble_publish_raw(protobuf_event_t event)
{

    NRF_LOG_DEBUG("publish raw: %s %s", event.name.bytes, event.data.bytes);

    // Encode value
    pb_byte_t output[protobuf_event_t_size];

    // Output buffer
    pb_ostream_t ostream = pb_ostream_from_buffer(output, sizeof(output));

    if (!pb_encode(&ostream, protobuf_event_t_fields, &event))
    {
        NRF_LOG_ERROR("Unable to encode: %s", PB_GET_ERROR(&ostream));
        return;
    }

    // TODO: send to connected device(s)
    switch (m_config.mode)
    {
        case ble_mode_peripheral:
            ble_peripheral_write(output, ostream.bytes_written);
            break;
        case ble_mode_central:
            ble_central_write(output, ostream.bytes_written);
            break;
    }
}

void ble_subscribe(char *name, susbcribe_handler_t handler)
{

    uint8_t name_length = strlen(name);

    // Check size
    if (name_length > member_size(protobuf_event_t_name_t, bytes))
    {
        NRF_LOG_WARNING("Name must be <= %d characters.", member_size(protobuf_event_t_name_t, bytes));
        return;
    }

    // Check subscription amount
    if (m_subscribe_list.count >= BLE_M_SUBSCRIBER_MAX_COUNT)
    {
        NRF_LOG_WARNING("Too many subscriptions.");
        return;
    }

    ble_subscription_handler_t subscriber = {
        .evt_handler = handler};

    // Copy over info to structure.
    subscriber.name.size = name_length;
    memcpy(subscriber.name.bytes, name, name_length);

    int index = 0;
    bool found = false;

    // Check if exists
    for (; index < m_subscribe_list.count; index++)
    {
        protobuf_event_t_name_t *name = &m_subscribe_list.subscribers[index].name;

        if (name->size == subscriber.name.size)
        {
            if (memcmp(name->bytes, subscriber.name.bytes, name->size) == 0)
            {
                found = true;
                break;
            }
        }
    }

    // Update the found one.
    if (found)
    {
        m_subscribe_list.subscribers[index] = subscriber;
    }
    // Otherwise create a new one
    else
    {
        m_subscribe_list.subscribers[m_subscribe_list.count] = subscriber;
        m_subscribe_list.count++;
    }
}

void advertising_start(void)
{

    if (m_config.mode == ble_mode_peripheral)
    {
        ble_peripheral_advertising_start();
    }
    else
    {
        NRF_LOG_WARNING("Unable to start advertising in central mode.");
    }
}

void scan_start(void)
{
    if (m_config.mode == ble_mode_central)
    {
        ble_central_scan_start();
    }
    else
    {
        NRF_LOG_WARNING("Unable to start scanning in peripheral mode.");
    }
}

/**@brief Function for handling BLE events.
 *
 * @param[in]   p_ble_evt   Bluetooth stack event.
 * @param[in]   p_context   Unused.
 */

static void ble_evt_handler(ble_evt_t const *p_ble_evt, void *p_context)
{

    // TODO: enqueue this in the main context as well.

    switch (m_config.mode)
    {
        case ble_mode_peripheral:
            ble_peripheral_evt_handler(p_ble_evt, p_context);
            break;
        case ble_mode_central:
            ble_central_evt_handler(p_ble_evt, p_context);
            break;
    }
}

/**@brief Function for the GAP initialization.
 *
 * @details This function sets up all the necessary GAP (Generic Access Profile) parameters of the
 *          device including the device name, appearance, and the preferred connection parameters.
 */
static void gap_params_init(void)
{
    uint32_t err_code;
    ble_gap_conn_params_t gap_conn_params;
    ble_gap_conn_sec_mode_t sec_mode;

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

    err_code = sd_ble_gap_device_name_set(&sec_mode,
                                          (const uint8_t *)DEVICE_NAME,
                                          strlen(DEVICE_NAME));
    APP_ERROR_CHECK(err_code);

    memset(&gap_conn_params, 0, sizeof(gap_conn_params));

    gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
    gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;
    gap_conn_params.slave_latency = SLAVE_LATENCY;
    gap_conn_params.conn_sup_timeout = CONN_SUP_TIMEOUT;

    err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for initializing the GATT module.
 */
static void gatt_init(void)
{
    ret_code_t err_code = nrf_ble_gatt_init(&m_gatt, NULL);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for queuing events so they can read in main context.
 */
static void ble_raw_evt_handler(protobuf_event_t *evt)
{

    // Forward to raw handler if it exists
    if (m_raw_handler_ext != NULL)
    {
        m_raw_handler_ext(evt);
    }
    // TODO: queue events.

    // TODO: adding \0 terminating char so printing, strlen, etc works
}

// TODO: transmit power
void ble_stack_init(ble_stack_init_t *init)
{
    ret_code_t err_code;

    // Copy over configuration
    m_config = *init;

    // Enable request for BLE stack
    err_code = nrf_sdh_enable_request();
    APP_ERROR_CHECK(err_code);

    // Configure the BLE stack using the default settings.
    // Fetch the start address of the application RAM.
    uint32_t ram_start = 0;
    err_code = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
    APP_ERROR_CHECK(err_code);

    // Enable BLE stack.
    err_code = nrf_sdh_ble_enable(&ram_start);
    APP_ERROR_CHECK(err_code);

    // Register handlers for BLE and SoC events.
    NRF_SDH_BLE_OBSERVER(m_ble_observer, APP_BLE_OBSERVER_PRIO, ble_evt_handler, NULL);

    gatt_init();
    gap_params_init();

    switch (m_config.mode)
    {
        case ble_mode_peripheral:
            // Attach handler
            ble_peripheral_attach_raw_handler(ble_raw_evt_handler);

            // Init peripheral mode
            ble_peripheral_init();
            break;

        case ble_mode_central:
            // First, attach handler
            ble_central_attach_raw_handler(ble_raw_evt_handler);

            // Initialize
            ble_central_init(&m_config.config);
            break;
    }
}

// Passthrough function for subscribing to RAW events
void ble_subscribe_raw(raw_susbcribe_handler_t handler)
{

    m_raw_handler_ext = handler;
}

// TODO: deque messages, fire off the appropriate handlers
void ble_process()
{
}