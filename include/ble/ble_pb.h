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
/** @file
 *
 * @defgroup ble_protobuf Protobuf Service
 * @{
 * @ingroup ble_sdk_srv
 * @brief Protobuf Service module.
 *
 * @details This module implements the Protobuf Service with the Command characteristic.
 *
 * @note    The application must register this module as BLE event observer using the
 *          NRF_SDH_BLE_OBSERVER macro. Example:
 *          @code
 *              ble_protobuf_t instance;
 *              NRF_SDH_BLE_OBSERVER(anything, BLE_PB_BLE_OBSERVER_PRIO,
 *                                   ble_protobuf_on_ble_evt, &instance);
 *          @endcode
 *
 */
#ifndef BLE_PB_H__
#define BLE_PB_H__

#include "ble.h"
#include "ble_srv_common.h"
#include "command.pb.h"
#include "nrf_sdh_ble.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

// UUID for the Service & Char
#define PROTOBUF_UUID_BASE                                 \
    {                                                      \
        0x72, 0x09, 0x1a, 0xb3, 0x5f, 0xff, 0x4d, 0xf6,    \
            0x80, 0x62, 0x45, 0x8d, 0x00, 0x00, 0x00, 0x00 \
    }
#define PROTOBUF_UUID_SERVICE 0xf510
#define PROTOBUF_UUID_CONFIG_CHAR (PROTOBUF_UUID_SERVICE + 1)

/**@brief Macro for defining a ble_protobuf instance.
 *
 * @param   _name  Name of the instance.
 * @hideinitializer
 */
#define BLE_PB_DEF(_name)                          \
    static ble_protobuf_t _name;                   \
    NRF_SDH_BLE_OBSERVER(_name##_obs,              \
                         BLE_PB_BLE_OBSERVER_PRIO, \
                         ble_protobuf_on_ble_evt,  \
                         &_name)

    // TODO: handling subscriptions

    /**@brief Heart Rate Service event type. */
    typedef enum
    {
        BLE_PB_EVT_DATA,
        BLE_PB_EVT_NOTIFICATION_ENABLED, /**< Heart Rate value notification enabled event. */
        BLE_PB_EVT_NOTIFICATION_DISABLED /**< Heart Rate value notification disabled event. */
    } ble_pb_evt_type_t;

    /**@brief Heart Rate Service event. */
    typedef struct
    {
        ble_pb_evt_type_t evt_type; /**< Type of event. */
    } ble_pb_evt_t;

    // Forward declaration of the ble_protobuf_t type.
    typedef struct ble_protobuf_s ble_protobuf_t;

    /**@brief Protobuf Service event handler type. */
    typedef void (*ble_protobuf_evt_handler_t)(ble_protobuf_t *p_protobuf, ble_pb_evt_t *p_evt, protobuf_event_t *p_pb_evt);

    /**@brief Protobuf Service init structure. This contains all options and data needed for
 *        initialization of the service.*/
    typedef struct
    {
        ble_protobuf_evt_handler_t evt_handler; /**< Event handler to be called for handling events in the Protobuf Service. */
        security_req_t bl_cccd_wr_sec;          /**< Security requirement for writing the BL characteristic CCCD. */
        security_req_t bl_wr_sec;               /**< Security requirement for writing the BL characteristic value */
    } ble_protobuf_init_t;

    /**@brief Protobuf Service structure. This contains various status information for the service. */
    struct ble_protobuf_s
    {
        ble_protobuf_evt_handler_t evt_handler;   /**< Event handler to be called for handling events in the Protobuf Service. */
        uint16_t service_handle;                  /**< Handle of Protobuf Service (as provided by the BLE stack). */
        ble_gatts_char_handles_t command_handles; /**< Handles related to the Command characteristic. */
        //TODO: Above is RX handle. Need a TX handle.
        uint8_t uuid_type;    /**< UUID type for the Protobuf Service. */
        uint16_t conn_handle; /**< Handle of the current connection (as provided by the BLE stack, is BLE_CONN_HANDLE_INVALID if not in a connection). */
    };

    // TODO: fill out more info on this call
    void ble_protobuf_write(ble_protobuf_t *p_protobuf, uint8_t *data, size_t size);

    /**@brief Function for initializing the Protobuf Service.
 *
 * @param[out]  p_protobuf       Protobuf Service structure. This structure will have to be supplied by
 *                          the application. It will be initialized by this function, and will later
 *                          be used to identify this particular service instance.
 * @param[in]   p_protobuf_init  Information needed to initialize the service.
 *
 * @return      NRF_SUCCESS on successful initialization of service, otherwise an error code.
 */
    ret_code_t ble_protobuf_init(ble_protobuf_t *p_protobuf, const ble_protobuf_init_t *p_protobuf_init);

    /**@brief Function for handling the Application's BLE Stack events.
 *
 * @details Handles all events from the BLE stack of interest to the Protobuf Service.
 *
 * @note For the requirements in the BAS specification to be fulfilled,
 *       ble_protobuf_command_update() must be called upon reconnection if the
 *       battery level has changed while the service has been disconnected from a bonded
 *       client.
 *
 * @param[in]   p_ble_evt   Event received from the BLE stack.
 * @param[in]   p_context   Protobuf Service structure.
 */
    void ble_protobuf_on_ble_evt(ble_evt_t const *p_ble_evt, void *p_context);

#ifdef __cplusplus
}
#endif

#endif // BLE_PB_H__

/** @} */
