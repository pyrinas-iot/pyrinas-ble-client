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

/**@brief     Application BLE module.
 *
 * @details   This module contains most of the functions used
 *            by the application to manage BLE stack events
 *            and BLE connections.
 */

#ifndef BLE_M_H__
#define BLE_M_H__

#include <stdbool.h>
#include <stdint.h>

#include "ble_central.h"
#include "ble_handlers.h"

#include "peer_manager.h"

#include "command.pb.h"

//TODO: better place to define this
#define BLE_M_SUBSCRIBER_MAX_COUNT 12 /**< Max amount of potential subscriptions. */

/**@brief Struct for tracking callbacks
 */
typedef struct
{
    susbcribe_handler_t evt_handler;
    protobuf_event_t_name_t name;
} ble_subscription_handler_t;

typedef struct
{
    ble_subscription_handler_t subscribers[BLE_M_SUBSCRIBER_MAX_COUNT];
    uint8_t count;
} ble_subscription_list_t;

/**@brief Different device "modes"
 */
typedef enum
{
    ble_mode_peripheral = 0,
    ble_mode_central = 1,
} ble_mode_t;

/**@brief BLE callback function to main context.
 */

/**@brief Struct for initialization of BLE stack.
 */
typedef struct
{
    ble_mode_t mode;
    bool long_range;
    union {
        ble_central_init_t config;
    };

} ble_stack_init_t;

#define BLE_STACK_PERIPH_DEF(X) ble_stack_init_t X = {.mode = ble_mode_peripheral, .long_range = false}
#define BLE_STACK_CENTRAL_DEF(X) ble_stack_init_t X = {.mode = ble_mode_central, .long_range = false}

/**@brief Function for terminating connection with a BLE peripheral device.
 */
void ble_disconnect(void);

/**@brief Function for initializing the BLE stack.
 *
 * @details Initializes the SoftDevice and the BLE event interrupts.
 */
void ble_stack_init(ble_stack_init_t *init);

/**@brief Function for starting the scanning.
 */
void scan_start(void);

/**@brief Function for publishing.
 */
void ble_publish(char *name, char *data);

// TODO: document this
void ble_publish_raw(protobuf_event_t event);

// TODO: document this
void ble_subscribe(char *name, susbcribe_handler_t handler);

// TODO: document this
void ble_subscribe_raw(raw_susbcribe_handler_t handler);

// TODO: document this
void advertising_start(void);

/**@brief Function for checking the connection state.
 *
 * @retval true     If peripheral device is connected.
 * @retval false    If peripheral device is not connected.
 */
bool ble_is_connected(void);

/**@brief Function for obtaining connection handle.
 *
 * @return Returns connection handle.
 */
uint16_t ble_get_conn_handle(void);

// TODO: Document this
void ble_pm_evt_handler(pm_evt_t const *p_evt);

// TODO: document this
void ble_process();

#endif // BLE_M_H__
