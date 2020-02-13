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

#include "app_error.h"
#include "boards.h"
#include "fds.h"
#include "nordic_common.h"

#include "ble_central.h"
#include "ble_m.h"
#include "ble_peripheral.h"

#include "nrf_ble_gatt.h"
#include "nrf_gpio.h"
#include "nrf_queue.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"

#include "util.h"

#include "pb_decode.h"
#include "pb_encode.h"

#define NRF_LOG_MODULE_NAME ble_m
#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();

#define APP_BLE_CONN_CFG_TAG 1  /**< Tag for the configuration of the BLE stack. */
#define APP_BLE_OBSERVER_PRIO 3 /**< BLE observer priority of the application. There is no need to modify this value. */

NRF_QUEUE_DEF(protobuf_event_t, m_event_queue, 20, NRF_QUEUE_MODE_OVERFLOW);

NRF_BLE_GATT_DEF(m_gatt); /**< GATT module instance. */

static ble_subscription_list_t m_subscribe_list; /**< Use for adding/removing subscriptions */

static ble_stack_init_t m_config; /**< Init config */

static raw_susbcribe_handler_t m_raw_handler_ext;

static int subscriber_search(protobuf_event_t_name_t *event_name); // Forward declaration of subscriber_search

bool ble_is_connected(void)
{

    bool is_connected = false;

    switch (m_config.mode)
    {
        case ble_mode_peripheral:
            is_connected = ble_peripheral_is_connected();
            break;
        case ble_mode_central:
            is_connected = ble_central_is_connected();
            break;
    }

    return is_connected;
}

void ble_disconnect(void)
{
    switch (m_config.mode)
    {
        case ble_mode_peripheral:
            ble_peripheral_disconnect();
            break;
        case ble_mode_central:
            ble_central_disconnect();
            break;
    }
}

void ble_publish(char *name, char *data)
{

    uint8_t name_length = strlen(name);
    uint8_t data_length = strlen(data);

    // Check size
    if (name_length >= member_size(protobuf_event_t_name_t, bytes))
    {
        NRF_LOG_WARNING("Name must be <= %d characters.", member_size(protobuf_event_t_name_t, bytes));
        return;
    }

    // Check size
    if (data_length >= member_size(protobuf_event_t_data_t, bytes))
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
    memcpy(event.name.bytes, name, name_length);
    memcpy(event.data.bytes, data, data_length);

    // Then publish it as a raw format.
    ble_publish_raw(event);
}

void ble_publish_raw(protobuf_event_t event)
{

    NRF_LOG_DEBUG("publish raw: %s %s", event.name.bytes, event.data.bytes);
    ble_gap_addr_t gap_addr;
    sd_ble_gap_addr_get(&gap_addr);

    // Copy over the address information
    memcpy(event.addr, gap_addr.addr, sizeof(event.addr));

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

    uint8_t name_length = strlen(name) + 1;

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

    // Check if exists
    int index = subscriber_search(&subscriber.name);

    // If index is >= 0, we have an entry
    if (index != -1)
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
        ble_peripheral_advertising_start(false);
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
    // Queue events.
    ret_code_t ret = nrf_queue_push(&m_event_queue, evt);
    APP_ERROR_CHECK(ret);
}

static void radio_switch_init()
{

    nrf_gpio_cfg_output(VCTL1);
    nrf_gpio_cfg_output(VCTL2);

    // VCTL2 low, Output 2
    // VCTL1 low, Output 1
    nrf_gpio_pin_clear(VCTL2);
    nrf_gpio_pin_set(VCTL1);
}

// TODO: transmit power
void ble_stack_init(ble_stack_init_t *init)
{
    ret_code_t err_code;

    // Copy over configuration
    m_config = *init;

    // First, let's set the output switch correctly..
    radio_switch_init();

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

// deque messages, fire off the appropriate handlers
void ble_process()
{

    // Dequeue one item if not empty
    if (!nrf_queue_is_empty(&m_event_queue))
    {

        static protobuf_event_t evt;
        ret_code_t ret = nrf_queue_pop(&m_event_queue, &evt);
        APP_ERROR_CHECK(ret);

        // Forward to raw handler if it exists
        if (m_raw_handler_ext != NULL)
        {
            m_raw_handler_ext(&evt);
        }

        // adding \0 terminating char so printing, strlen, etc works
        evt.name.bytes[evt.name.size++] = 0;
        evt.data.bytes[evt.data.size++] = 0;

        // Check if exists
        int index = subscriber_search(&evt.name);

        // If index is >= 0, we have an entry
        if (index != -1)
        {
            // Push to susbscription context
            m_subscribe_list.subscribers[index].evt_handler((char *)evt.name.bytes, (char *)evt.data.bytes);
        }
    }
}

// TODO: more optimized way of doing this?
static int subscriber_search(protobuf_event_t_name_t *event_name)
{

    int index = 0;

    for (; index < m_subscribe_list.count; index++)
    {
        protobuf_event_t_name_t *name = &m_subscribe_list.subscribers[index].name;

        if (name->size == event_name->size)
        {
            if (memcmp(name->bytes, event_name->bytes, name->size) == 0)
            {
                return index;
            }
        }
    }

    return -1;
}

void ble_pm_evt_handler(pm_evt_t const *p_evt)
{
    switch (m_config.mode)
    {
        case ble_mode_peripheral:
            ble_peripheral_pm_evt_handler(p_evt);
            break;
        case ble_mode_central:
            ble_central_pm_evt_handler(p_evt);
            break;
    }
}