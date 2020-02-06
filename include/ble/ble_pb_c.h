/**
 * Copyright (c) 2012 - 2019, Nordic Semiconductor ASA
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
/**@file
 *
 * @defgroup ble_pb_c Protobuf Service Client
 * @{
 * @ingroup  ble_sdk_srv
 * @brief    Protobuf Service Client module.
 *
 * @details  This module contains the APIs and types exposed by the Protobuf Service Client
 *           module. The application can use these APIs and types to perform the discovery of
 *           Protobuf Service at the peer and to interact with it.
 *
 * @note    The application must register this module as the BLE event observer by using the
 *          NRF_SDH_BLE_OBSERVER macro. Example:
 *          @code
 *              ble_pb_c_t instance;
 *              NRF_SDH_BLE_OBSERVER(anything, BLE_PB_C_BLE_OBSERVER_PRIO,
 *                                   ble_pb_c_on_ble_evt, &instance);
 *          @endcode
 */

#ifndef BLE_PB_C_H__
#define BLE_PB_C_H__

#include "ble.h"
#include "ble_db_discovery.h"
#include "ble_srv_common.h"
#include "command.pb.h"
#include "nrf_ble_gq.h"
#include "nrf_sdh_ble.h"
#include "sdk_config.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define PB_SERVICE_UUID_TYPE BLE_UUID_TYPE_VENDOR_BEGIN /**< UUID type for the PB Service (vendor specific). */

/**@brief   Macro for defining a ble_pb_c instance.
 *
 * @param   _name   Name of the instance.
 * @hideinitializer
 */
#define BLE_PB_C_DEF(_name)                          \
    static ble_pb_c_t _name;                         \
    NRF_SDH_BLE_OBSERVER(_name##_obs,                \
                         BLE_PB_C_BLE_OBSERVER_PRIO, \
                         ble_pb_c_on_ble_evt, &_name)

    /**
 * @defgroup pb_c_enums Enumerations
 * @{
 */

    /**@brief PB Client event type. */
    typedef enum
    {
        BLE_PB_C_EVT_DISCOVERY_COMPLETE = 1, /**< Event indicating that the Protobuf Service was discovered at the peer. */
        BLE_PB_C_EVT_NOTIFICATION            /**< Event indicating that a notification of the Protobuf characteristic was received from the peer. */
    } ble_pb_c_evt_type_t;

    /** @} */

    /**
 * @defgroup pb_c_structs Structures
 * @{
 */

    /**@brief Structure containing the handles related to the Protobuf Service found on the peer. */
    typedef struct
    {
        uint16_t pb_cccd_handle; /**< Handle of the CCCD of the Protobuf characteristic. */
        uint16_t pb_handle;      /**< Handle of the Protobuf characteristic, as provided by the SoftDevice. */
    } pb_db_t;

    /**@brief Protobuf Event structure. */
    typedef struct
    {
        ble_pb_c_evt_type_t evt_type; /**< Type of the event. */
        uint16_t conn_handle;         /**< Connection handle on which the Protobuf service was discovered on the peer device..*/
        union {
            pb_db_t peer_db;     /**< Handles related to the Protobuf, found on the peer device. This is filled if the evt_type is @ref BLE_PB_C_EVT_DISCOVERY_COMPLETE.*/
            protobuf_event_t pb; /**< Protobuf received. This is filled if the evt_type is @ref BLE_PB_C_EVT_NOTIFICATION. */
        } params;
    } ble_pb_c_evt_t;

    /** @} */

    /**
 * @defgroup pb_c_types Types
 * @{
 */

    // Forward declaration of the ble_bas_t type.
    typedef struct ble_pb_c_s ble_pb_c_t;

    /**@brief   Event handler type.
 *
 * @details This is the type of the event handler that is to be provided by the application
 *          of this module to receive events.
 */
    typedef void (*ble_pb_c_evt_handler_t)(ble_pb_c_t *p_pb_c, ble_pb_c_evt_t *p_evt, protobuf_event_t *p_pb_evt);

    /** @} */

    /**
 * @addtogroup pb_c_structs
 * @{
 */

    /**@brief Protobuf Client structure.
 */
    struct ble_pb_c_s
    {
        uint8_t uuid_type;                     /**< UUID type for DFU UUID. */
        uint16_t conn_handle;                  /**< Connection handle, as provided by the SoftDevice. */
        pb_db_t peer_pb_db;                    /**< Handles related to PB on the peer. */
        ble_pb_c_evt_handler_t evt_handler;    /**< Application event handler to be called when there is an event related to the Protobuf Service. */
        ble_srv_error_handler_t error_handler; /**< Function to be called in case of an error. */
        nrf_ble_gq_t *p_gatt_queue;            /**< Pointer to the BLE GATT Queue instance. */
    };

    /**@brief Protobuf Client initialization structure.
 */
    typedef struct
    {
        ble_pb_c_evt_handler_t evt_handler;    /**< Event handler to be called by the Protobuf Client module when there is an event related to the Protobuf Service. */
        ble_srv_error_handler_t error_handler; /**< Function to be called in case of an error. */
        nrf_ble_gq_t *p_gatt_queue;            /**< Pointer to the BLE GATT Queue instance. */
    } ble_pb_c_init_t;

    /** @} */

    /**
 * @defgroup pb_c_functions Functions
 * @{
 */

    // TODO: document this
    uint32_t ble_pb_c_write(ble_pb_c_t *p_ble_pb_c, uint8_t *data, size_t size);

    /**@brief     Function for initializing the Protobuf Client module.
 *
 * @details   This function registers with the Database Discovery module for the Protobuf Service.
 *		   	  The module looks for the presence of a Protobuf Service instance at the peer
 *            when a discovery is started.
 *
 * @param[in] p_ble_pb_c      Pointer to the Protobuf Client structure.
 * @param[in] p_ble_pb_c_init Pointer to the Protobuf initialization structure that contains
 *                             the initialization information.
 *
 * @retval    NRF_SUCCESS On successful initialization.
 * @retval    err_code    Otherwise, this function propagates the error code returned by the Database Discovery module API
 *                        @ref ble_db_discovery_evt_register.
 */
    uint32_t ble_pb_c_init(ble_pb_c_t *p_ble_pb_c, ble_pb_c_init_t *p_ble_pb_c_init);

    /**@brief     Function for handling BLE events from the SoftDevice.
 *
 * @details   This function handles the BLE events received from the SoftDevice. If a BLE
 *            event is relevant to the Protobuf Client module, the function uses the event's data to update
 *            interval variables and, if necessary, send events to the application.
 *
 * @param[in] p_ble_evt     Pointer to the BLE event.
 * @param[in] p_context     Pointer to the Protobuf Client structure.
 */
    void ble_pb_c_on_ble_evt(ble_evt_t const *p_ble_evt, void *p_context);

    /**@brief   Function for requesting the peer to start sending notification of Protobuf
 *          Measurement.
 *
 * @details This function enables notification of the Protobuf at the peer
 *          by writing to the CCCD of the Protobuf characteristic.
 *
 * @param   p_ble_pb_c Pointer to the Protobuf Client structure.
 *
 * @retval  NRF_SUCCESS If the SoftDevice is requested to write to the CCCD of the peer.
 * @retval	err_code	Otherwise, this function propagates the error code returned
 *                      by the SoftDevice API @ref sd_ble_gattc_write.
 */
    uint32_t ble_pb_c_notif_enable(ble_pb_c_t *p_ble_pb_c);

    /**@brief     Function for handling events from the Database Discovery module.
 *
 * @details   Call this function when you get a callback event from the Database Discovery module.
 *            This function handles an event from the Database Discovery module and determines
 *            whether it relates to the discovery of Protobuf Service at the peer. If it does, the function
 *            calls the application's event handler to indicate that the Protobuf Service was
 *            discovered at the peer. The function also populates the event with service-related
 *            information before providing it to the application.
 *
 * @param[in] p_ble_pb_c Pointer to the Protobuf Client structure instance for associating the link.
 * @param[in] p_evt Pointer to the event received from the Database Discovery module.
 *
 */
    void ble_pb_on_db_disc_evt(ble_pb_c_t *p_ble_pb_c, const ble_db_discovery_evt_t *p_evt);

    /**@brief     Function for assigning handles to an instance of pb_c.
 *
 * @details   Call this function when a link has been established with a peer to
 *            associate the link to this instance of the module. This association makes it
 *            possible to handle several links and associate each link to a particular
 *            instance of this module. The connection handle and attribute handles are
 *            provided from the discovery event @ref BLE_PB_C_EVT_DISCOVERY_COMPLETE.
 *
 * @param[in] p_ble_pb_c        Pointer to the Protobuf Client structure instance for associating the link.
 * @param[in] conn_handle        Connection handle to associate with the given Protobuf Client Instance.
 * @param[in] p_peer_pb_handles Attribute handles for the PB server you want this PB_C client to
 *                               interact with.
 */
    uint32_t ble_pb_c_handles_assign(ble_pb_c_t *p_ble_pb_c,
                                     uint16_t conn_handle,
                                     const pb_db_t *p_peer_pb_handles);

    /** @} */ // End tag for Function group.

#ifdef __cplusplus
}
#endif

#endif // BLE_PB_C_H__

/** @} */ // End tag for the file.
