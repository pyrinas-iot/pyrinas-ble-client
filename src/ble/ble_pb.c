/**
 * Copyright (c) 2012 - 2018, Nordic Semiconductor ASA
 * Copyright (c) 2019, Jared Wolff
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
/* Attention!
 * To maintain compliance with Nordic Semiconductor ASA's Bluetooth profile
 * qualification listings, this section of source code must not be modified.
 */
#include "sdk_common.h"
#if NRF_MODULE_ENABLED(BLE_PB)
#include "app_error.h"
#include "ble_conn_state.h"
#include "ble_pb.h"
#include "ble_srv_common.h"
#include "command.pb.h"
#include "pb_decode.h"
#include "pb_encode.h"
#include <string.h>

// TODO: rename ble_protobuf to ble_pb
#define NRF_LOG_MODULE_NAME ble_protobuf
#if BLE_PB_CONFIG_LOG_ENABLED
#define NRF_LOG_LEVEL BLE_PB_CONFIG_LOG_LEVEL
#define NRF_LOG_INFO_COLOR BLE_PB_CONFIG_INFO_COLOR
#define NRF_LOG_DEBUG_COLOR BLE_PB_CONFIG_DEBUG_COLOR
#else // BLE_PB_CONFIG_LOG_ENABLED
#define NRF_LOG_LEVEL 0
#endif // BLE_PB_CONFIG_LOG_ENABLED
#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();

/**@brief Function for handling write events to the Heart Rate Measurement characteristic.
 *
 * @param[in]   p_protobuf         Heart Rate Service structure.
 * @param[in]   p_evt_write   Write event received from the BLE stack.
 */
static void on_pb_cccd_write(ble_protobuf_t *p_protobuf, ble_gatts_evt_write_t const *p_evt_write)
{
    if (p_evt_write->len == 2)
    {
        // CCCD written, update notification state
        if (p_protobuf->evt_handler != NULL)
        {
            ble_pb_evt_t evt;

            if (ble_srv_is_notification_enabled(p_evt_write->data))
            {
                evt.evt_type = BLE_PB_EVT_NOTIFICATION_ENABLED;
                NRF_LOG_DEBUG("Notifications enabled!");
            }
            else
            {
                NRF_LOG_DEBUG("Notifications disabled!");
                evt.evt_type = BLE_PB_EVT_NOTIFICATION_DISABLED;
            }

            p_protobuf->evt_handler(p_protobuf, &evt);
        }
    }
}

/**@brief Function for handling the Write event.
 *
 * @param[in]   p_protobuf       Battery Service structure.
 * @param[in]   p_ble_evt   Event received from the BLE stack.
 */
static void on_write(ble_protobuf_t *p_protobuf, ble_evt_t const *p_ble_evt)
{
    ble_gatts_evt_write_t const *p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;

    if (p_evt_write->handle == p_protobuf->command_handles.cccd_handle)
    {
        on_pb_cccd_write(p_protobuf, p_evt_write);
    }

    // Handle writning to the value handle
    if (p_evt_write->handle == p_protobuf->command_handles.value_handle)
    {

        // Setitng up protocol buffer data
        protobuf_event_t data;

        ble_pb_evt_t evt;
        evt.evt_type = BLE_PB_EVT_DATA;

        // NRF_LOG_HEXDUMP_INFO(p_evt_write->data,p_evt_write->len);

        // Read in buffer
        pb_istream_t istream = pb_istream_from_buffer((pb_byte_t *)p_evt_write->data, p_evt_write->len);

        if (!pb_decode(&istream, protobuf_event_t_fields, &data))
        {
            NRF_LOG_ERROR("Unable to decode: %s", PB_GET_ERROR(&istream));
            return;
        }

        // Copy data over
        evt.params.data = data;

        // Event to the main context
        p_protobuf->evt_handler(p_protobuf, &evt);
    }
}

// TODO: add comment
uint32_t ble_protobuf_write(ble_protobuf_t *p_protobuf, uint8_t *data, size_t size)
{

    ret_code_t err_code;

    // Send value if connected and notifying
    if (p_protobuf->conn_handle != BLE_CONN_HANDLE_INVALID)
    {

        uint16_t hvx_len = size;
        ble_gatts_hvx_params_t hvx_params;

        memset(&hvx_params, 0, sizeof(hvx_params));

        hvx_params.handle = p_protobuf->command_handles.value_handle;
        hvx_params.type = BLE_GATT_HVX_NOTIFICATION;
        hvx_params.offset = 0;
        hvx_params.p_len = &hvx_len;
        hvx_params.p_data = data;

        err_code = sd_ble_gatts_hvx(p_protobuf->conn_handle, &hvx_params);
    }
    else
    {
        err_code = NRF_ERROR_INVALID_STATE;
    }

    return err_code;
}

/**@brief Function for handling the Disconnect event.
 *
 * @param[in]   p_protobuf       Heart Rate Service structure.
 * @param[in]   p_ble_evt   Event received from the BLE stack.
 */
static void on_disconnect(ble_protobuf_t *p_protobuf, ble_evt_t const *p_ble_evt)
{
    UNUSED_PARAMETER(p_ble_evt);
    p_protobuf->conn_handle = BLE_CONN_HANDLE_INVALID;
}

/**@brief Function for handling the Connect event.
 *
 * @param[in]   p_protobuf       Heart Rate Service structure.
 * @param[in]   p_ble_evt   Event received from the BLE stack.
 */
static void on_connect(ble_protobuf_t *p_protobuf, ble_evt_t const *p_ble_evt)
{
    p_protobuf->conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
}

void ble_protobuf_on_ble_evt(ble_evt_t const *p_ble_evt, void *p_context)
{
    if ((p_context == NULL) || (p_ble_evt == NULL))
    {
        return;
    }

    ble_protobuf_t *p_protobuf = (ble_protobuf_t *)p_context;

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
            on_connect(p_protobuf, p_ble_evt);
            break;
        case BLE_GAP_EVT_DISCONNECTED:
            on_disconnect(p_protobuf, p_ble_evt);
            break;
        case BLE_GATTS_EVT_WRITE:
            on_write(p_protobuf, p_ble_evt);
            break;

        default:
            // No implementation needed.
            break;
    }
}

/**@brief Function for adding the Battery Level characteristic.
 *
 * @param[in]   p_protobuf        Battery Service structure.
 * @param[in]   p_protobuf_init   Information needed to initialize the service.
 *
 * @return      NRF_SUCCESS on success, otherwise an error code.
 */
static ret_code_t command_char_add(ble_protobuf_t *p_protobuf, const ble_protobuf_init_t *p_protobuf_init)
{
    ret_code_t err_code;
    ble_add_char_params_t add_char_params;

    memset(&add_char_params, 0, sizeof(add_char_params));

    add_char_params.uuid = PROTOBUF_UUID_CONFIG_CHAR;
    add_char_params.max_len = protobuf_event_t_size;
    add_char_params.is_var_len = true;

    add_char_params.char_props.notify = 1;
    add_char_params.char_props.write = 1;

    add_char_params.cccd_write_access = p_protobuf_init->bl_cccd_wr_sec;
    add_char_params.write_access = p_protobuf_init->bl_wr_sec;

    err_code = characteristic_add(p_protobuf->service_handle,
                                  &add_char_params,
                                  &(p_protobuf->command_handles));
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    return NRF_SUCCESS;
}

ret_code_t ble_protobuf_init(ble_protobuf_t *p_protobuf, const ble_protobuf_init_t *p_protobuf_init)
{
    if (p_protobuf == NULL || p_protobuf_init == NULL)
    {
        return NRF_ERROR_NULL;
    }

    ret_code_t err_code;
    ble_uuid_t ble_uuid;
    ble_uuid128_t base_uuid = {PROTOBUF_UUID_BASE};

    // Initialize service structure
    p_protobuf->evt_handler = p_protobuf_init->evt_handler;
    p_protobuf->conn_handle = BLE_CONN_HANDLE_INVALID;

    // Add service
    err_code = sd_ble_uuid_vs_add(&base_uuid, &p_protobuf->uuid_type);
    VERIFY_SUCCESS(err_code);

    ble_uuid.type = p_protobuf->uuid_type;
    ble_uuid.uuid = PROTOBUF_UUID_SERVICE;

    err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &ble_uuid, &p_protobuf->service_handle);
    VERIFY_SUCCESS(err_code);

    // Add battery level characteristic
    err_code = command_char_add(p_protobuf, p_protobuf_init);
    return err_code;
}

#endif // NRF_MODULE_ENABLED(BLE_PB)
