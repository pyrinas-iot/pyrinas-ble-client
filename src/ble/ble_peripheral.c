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
#include "bsp.h"

#include "ble_advdata.h"
#include "ble_advertising.h"
#include "ble_m.h"
#include "ble_pb.h"
#include "ble_peripheral.h"

#include "nrf_ble_qwr.h"

NRF_BLE_QWR_DEF(m_qwr);             /**< Context for the Queued Write module.*/
BLE_PB_DEF(m_protobuf);             /**< Protobuf module instance. */
BLE_ADVERTISING_DEF(m_advertising); /**< Advertising module instance. */

#define NRF_LOG_MODULE_NAME ble_m_periph
#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();

// Static defines
static uint16_t m_conn_handle = BLE_CONN_HANDLE_INVALID; /**< Handle of the current connection. */
static bool m_advertising_on_disconnect = true;
static bool m_connected = false;
static bool m_notifications_enabled = false;

static raw_susbcribe_handler_t m_raw_evt_handler = NULL;

static pm_peer_id_t m_peer_id; /**< Device reference handle to the current bonded central. */

/**@brief Function for setting filtered device identities.
 *
 * @param[in] skip  Filter passed to @ref pm_peer_id_list.
 */
static void identities_set(pm_peer_id_list_skip_t skip)
{
    pm_peer_id_t peer_ids[BLE_GAP_DEVICE_IDENTITIES_MAX_COUNT];
    uint32_t peer_id_count = BLE_GAP_DEVICE_IDENTITIES_MAX_COUNT;

    ret_code_t err_code = pm_peer_id_list(peer_ids, &peer_id_count, PM_PEER_ID_INVALID, skip);
    APP_ERROR_CHECK(err_code);

    err_code = pm_device_identities_list_set(peer_ids, peer_id_count);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for handling BLE events.
 *
 * @param[in]   p_ble_evt   Bluetooth stack event.
 * @param[in]   p_context   Unused.
 */
void ble_peripheral_evt_handler(ble_evt_t const *p_ble_evt, void *p_context)
{
    uint32_t err_code = NRF_SUCCESS;

    switch (p_ble_evt->header.evt_id)
    {
    case BLE_GAP_EVT_DISCONNECTED:
        NRF_LOG_INFO("Peripheral disconnected!");

        // Set LED
        err_code = bsp_indication_set(BSP_INDICATE_IDLE);
        APP_ERROR_CHECK(err_code);

        // Reset variables.
        m_conn_handle = BLE_CONN_HANDLE_INVALID;
        m_connected = false;
        m_notifications_enabled = false;

        // Only advertising on disconnect when enabled
        if (m_advertising_on_disconnect)
            ble_peripheral_advertising_start(false);

        break;

    case BLE_GAP_EVT_CONNECTED:
        // Set connected flag
        m_connected = true;

        // Set LED
        err_code = bsp_indication_set(BSP_INDICATE_BONDING);
        APP_ERROR_CHECK(err_code);

        // Set connection handle
        m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;

        // Assign QWR
        err_code = nrf_ble_qwr_conn_handle_assign(&m_qwr, m_conn_handle);
        APP_ERROR_CHECK(err_code);

        break;

    case BLE_GAP_EVT_PHY_UPDATE_REQUEST:
    {
        NRF_LOG_DEBUG("PHY update request.");
        ble_gap_phys_t const phys =
            {
                .rx_phys = BLE_GAP_PHY_AUTO,
                .tx_phys = BLE_GAP_PHY_AUTO,
            };
        err_code = sd_ble_gap_phy_update(p_ble_evt->evt.gap_evt.conn_handle, &phys);
        APP_ERROR_CHECK(err_code);
        break;
    }

    case BLE_GATTC_EVT_TIMEOUT:
        // Disconnect on GATT Client timeout event.
        NRF_LOG_DEBUG("GATT Client Timeout.");
        err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gattc_evt.conn_handle,
                                         BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
        APP_ERROR_CHECK(err_code);
        break;

    case BLE_GATTS_EVT_TIMEOUT:
        // Disconnect on GATT Server timeout event.
        NRF_LOG_DEBUG("GATT Server Timeout.");
        err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gatts_evt.conn_handle,
                                         BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
        APP_ERROR_CHECK(err_code);
        break;
    default:
        // No implementation needed.
        break;
    }
}

/**@brief Function for setting filtered whitelist.
 *
 * @param[in] skip  Filter passed to @ref pm_peer_id_list.
 */
static void whitelist_set(pm_peer_id_list_skip_t skip)
{
    pm_peer_id_t peer_ids[BLE_GAP_WHITELIST_ADDR_MAX_COUNT];
    uint32_t peer_id_count = BLE_GAP_WHITELIST_ADDR_MAX_COUNT;

    ret_code_t err_code = pm_peer_id_list(peer_ids, &peer_id_count, PM_PEER_ID_INVALID, skip);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_DEBUG("\tm_whitelist_peer_cnt %d, MAX_PEERS_WLIST %d",
                  peer_id_count + 1,
                  BLE_GAP_WHITELIST_ADDR_MAX_COUNT);

    err_code = pm_whitelist_set(peer_ids, peer_id_count);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for starting advertising.
 */
void ble_peripheral_advertising_start(bool erase_bonds)
{

    ret_code_t err_code;

    if (erase_bonds == true)
    {
        err_code = pm_peers_delete();
        APP_ERROR_CHECK(err_code);
        // Advertising is started by PM_EVT_PEERS_DELETE_SUCCEEDED event.
    }
    else
    {
        // Be double sure advertising is stopped
        sd_ble_gap_adv_stop(m_advertising.adv_handle);

        // Then set whitelist
        whitelist_set(PM_PEER_ID_LIST_SKIP_NO_ID_ADDR);

        // Then, start advertising!
        err_code = ble_advertising_start(&m_advertising, BLE_ADV_MODE_FAST);
        APP_ERROR_CHECK(err_code);
    }
}

static void advertising_config_get(ble_adv_modes_config_t *p_config)
{
    memset(p_config, 0, sizeof(ble_adv_modes_config_t));

    p_config->ble_adv_on_disconnect_disabled = true;
    p_config->ble_adv_whitelist_enabled = true;
    p_config->ble_adv_directed_enabled = false;
    p_config->ble_adv_extended_enabled = true;
    p_config->ble_adv_fast_enabled = true;
    p_config->ble_adv_fast_interval = APP_ADV_INTERVAL;
    p_config->ble_adv_fast_timeout = APP_ADV_DURATION;

    // Coded PHY
    p_config->ble_adv_primary_phy = BLE_GAP_PHY_CODED;
    p_config->ble_adv_secondary_phy = BLE_GAP_PHY_CODED;
}

/**@brief Function for handling advertising events.
 *
 * @details This function will be called for advertising events which are passed to the application.
 *
 * @param[in] ble_adv_evt  Advertising event.
 */
static void on_adv_evt(ble_adv_evt_t ble_adv_evt)
{
    uint32_t err_code;

    switch (ble_adv_evt)
    {
    case BLE_ADV_EVT_FAST:
        err_code = bsp_indication_set(BSP_INDICATE_ADVERTISING);
        APP_ERROR_CHECK(err_code);
        break;

    case BLE_ADV_EVT_IDLE:
        break;
    case BLE_ADV_EVT_WHITELIST_REQUEST:
    {
        ble_gap_addr_t whitelist_addrs[BLE_GAP_WHITELIST_ADDR_MAX_COUNT];
        ble_gap_irk_t whitelist_irks[BLE_GAP_WHITELIST_ADDR_MAX_COUNT];
        uint32_t addr_cnt = BLE_GAP_WHITELIST_ADDR_MAX_COUNT;
        uint32_t irk_cnt = BLE_GAP_WHITELIST_ADDR_MAX_COUNT;

        err_code = pm_whitelist_get(whitelist_addrs, &addr_cnt,
                                    whitelist_irks, &irk_cnt);
        APP_ERROR_CHECK(err_code);
        NRF_LOG_DEBUG("pm_whitelist_get returns %d addr in whitelist and %d irk whitelist",
                      addr_cnt, irk_cnt);

        // Set the correct identities list (no excluding peers with no Central Address Resolution).
        identities_set(PM_PEER_ID_LIST_SKIP_NO_IRK);

        // Apply the whitelist.
        err_code = ble_advertising_whitelist_reply(&m_advertising,
                                                   whitelist_addrs,
                                                   addr_cnt,
                                                   whitelist_irks,
                                                   irk_cnt);
        APP_ERROR_CHECK(err_code);
    }
    break; //BLE_ADV_EVT_WHITELIST_REQUEST

    case BLE_ADV_EVT_PEER_ADDR_REQUEST:
    {
        pm_peer_data_bonding_t peer_bonding_data;

        // Only Give peer address if we have a handle to the bonded peer.
        if (m_peer_id != PM_PEER_ID_INVALID)
        {
            err_code = pm_peer_data_bonding_load(m_peer_id, &peer_bonding_data);
            if (err_code != NRF_ERROR_NOT_FOUND)
            {
                APP_ERROR_CHECK(err_code);

                // Manipulate identities to exclude peers with no Central Address Resolution.
                identities_set(PM_PEER_ID_LIST_SKIP_ALL);

                ble_gap_addr_t *p_peer_addr = &(peer_bonding_data.peer_ble_id.id_addr_info);
                err_code = ble_advertising_peer_addr_reply(&m_advertising, p_peer_addr);
                APP_ERROR_CHECK(err_code);
            }
        }
    }
    break; //BLE_ADV_EVT_PEER_ADDR_REQUEST
    default:
        break;
    }
}

/**@brief Function for handling advertising errors.
 *
 * @param[in] nrf_error  Error code containing information about what went wrong.
 */
static void ble_advertising_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}

// TODO: configure with whitelist
/**@brief Function for initializing the Advertising functionality.
 */
static void advertising_init(void)
{
    uint32_t err_code;
    ble_advertising_init_t init;

    int8_t tx_power = APP_ADV_TX_POWER;

    memset(&init, 0, sizeof(init));

    init.advdata.p_tx_power_level = &tx_power; //8dBm
    init.advdata.name_type = BLE_ADVDATA_FULL_NAME;
    init.advdata.include_appearance = true;
    init.advdata.flags = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;

    advertising_config_get(&init.config);

    init.evt_handler = on_adv_evt;
    init.error_handler = ble_advertising_error_handler;

    err_code = ble_advertising_init(&m_advertising, &init);
    APP_ERROR_CHECK(err_code);

    ble_advertising_conn_cfg_tag_set(&m_advertising, APP_BLE_CONN_CFG_TAG);
}

void ble_peripheral_pm_evt_handler(pm_evt_t const *p_evt)
{
    switch (p_evt->evt_id)
    {
    case PM_EVT_PEERS_DELETE_SUCCEEDED:
        // Re-enable advertising on disconnect
        m_advertising_on_disconnect = true;

        // Start advertising
        ble_peripheral_advertising_start(false);
        break;

    case PM_EVT_PEER_DATA_UPDATE_SUCCEEDED:
        if (p_evt->params.peer_data_update_succeeded.flash_changed && (p_evt->params.peer_data_update_succeeded.data_id == PM_PEER_DATA_ID_BONDING))
        {
            NRF_LOG_INFO("New Bond, add the peer to the whitelist if possible");
            // Note: You should check on what kind of white list policy your application should use.

            whitelist_set(PM_PEER_ID_LIST_SKIP_NO_ID_ADDR);
        }
        break;

    default:
        break;
    }
}

/**@brief Function for forwarding protobuf data from the BLE portion of things
 */
// TODO: kill this event handler and set one up in init
void ble_protobuf_evt_hanlder(ble_protobuf_t *p_protobuf, ble_pb_evt_t *p_evt)
{

    uint32_t err_code;

    switch (p_evt->evt_type)
    {
    case BLE_PB_EVT_NOTIFICATION_ENABLED:
        NRF_LOG_INFO("Notifications enabled!")
        m_notifications_enabled = true;

        err_code = bsp_indication_set(BSP_INDICATE_CONNECTED);
        APP_ERROR_CHECK(err_code);
        break;
    case BLE_PB_EVT_NOTIFICATION_DISABLED:
        NRF_LOG_INFO("Notifications disabled!")
        break;
    case BLE_PB_EVT_DATA:
        NRF_LOG_DEBUG("Data!");

        // Forward to raw handler.
        if (m_raw_evt_handler != NULL)
        {
            m_raw_evt_handler(&(p_evt->params.data));
        }

        break;
    }
}

/**@brief Function for handling Queued Write Module errors.
 *
 * @details A pointer to this function will be passed to each service which may need to inform the
 *          application about an error.
 *
 * @param[in]   nrf_error   Error code containing information about what went wrong.
 */
static void nrf_qwr_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}

/**@brief Function for initializing services that will be used by the application.
 */
static void services_init(void)
{
    uint32_t err_code;
    nrf_ble_qwr_init_t qwr_init = {0};
    ble_protobuf_init_t protobuf_init = {0};

    // Initialize Queued Write Module.
    qwr_init.error_handler = nrf_qwr_error_handler;

    err_code = nrf_ble_qwr_init(&m_qwr, &qwr_init);
    APP_ERROR_CHECK(err_code);

    protobuf_init.evt_handler = ble_protobuf_evt_hanlder;
    protobuf_init.bl_cccd_wr_sec = SEC_JUST_WORKS;
    protobuf_init.bl_wr_sec = SEC_JUST_WORKS;

    err_code = ble_protobuf_init(&m_protobuf, &protobuf_init);
    APP_ERROR_CHECK(err_code);
}

void ble_peripheral_write(uint8_t *data, size_t size)
{

    // Shoots out a warning that it's not connected.. yet.
    if (!m_connected)
    {
        NRF_LOG_WARNING("Unable to write. Not connected!");
        return;
    }

    // Shoots out a warning that it's not connected.. yet.
    if (!m_notifications_enabled)
    {
        NRF_LOG_WARNING("Unable to write. Notifications not enabled!");
        return;
    }

    // Otherwise writes the data.
    ret_code_t err_code = ble_protobuf_write(&m_protobuf, data, size);
    if (err_code == NRF_ERROR_INVALID_STATE || err_code == NRF_ERROR_FORBIDDEN ||
        err_code == NRF_ERROR_RESOURCES)
    {
        NRF_LOG_WARNING("Not connected. Unable to send message.");
    }
    else
    {
        APP_ERROR_CHECK(err_code);
    }
}

void ble_peripheral_init()
{
    advertising_init();
    services_init();
}

void ble_peripheral_attach_raw_handler(raw_susbcribe_handler_t raw_evt_handler)
{
    m_raw_evt_handler = raw_evt_handler;
}

void ble_peripheral_disconnect()
{

    // Disable advertising on disconnect.
    m_advertising_on_disconnect = false;

    // TODO: handle this better?
    if (m_conn_handle != BLE_CONN_HANDLE_INVALID)
    {
        NRF_LOG_INFO("Disconnect");
        ret_code_t err_code = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
        if (err_code != NRF_ERROR_INVALID_STATE)
        {
            APP_ERROR_CHECK(err_code);
        }
    }
}

bool ble_peripheral_is_connected(void)
{
    return m_connected;
}