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

#include "app_timer.h"
#include "bsp.h"
#include "util.h"

#include "ble_advdata.h"
#include "ble_central.h"
#include "ble_conn_state.h"
#include "ble_db_discovery.h"
#include "ble_m.h"
#include "ble_pb_c.h"

#include "nrf_ble_qwr.h"
#include "nrf_ble_scan.h"
#include "nrf_fstorage.h"
#include "nrf_sdh_soc.h"

#include "peer_manager.h"

#define NRF_LOG_MODULE_NAME ble_m_central
#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();

#define FIRST_CONN_PARAMS_UPDATE_DELAY APP_TIMER_TICKS(5000) /**< Time from initiating event (connect or start of notification) to first time sd_ble_gap_conn_param_update is called (5 seconds). */
#define NEXT_CONN_PARAMS_UPDATE_DELAY APP_TIMER_TICKS(30000) /**< Time between each call to sd_ble_gap_conn_param_update after the first call (30 seconds). */
#define MAX_CONN_PARAMS_UPDATE_COUNT 3                       /**< Number of attempts before giving up the connection parameter negotiation. */

#define APP_SOC_OBSERVER_PRIO 1 /**< SoC observer priority of the application. There is no need to modify this value. */
#define DEV_NAME_LEN ((BLE_GAP_ADV_SET_DATA_SIZE_MAX + 1) - \
                      AD_DATA_OFFSET) /**< Determines the device name length. */

BLE_DB_DISCOVERY_DEF(m_db_discovery);                  /**< Database Discovery module instance. */
NRF_BLE_SCAN_DEF(m_scan);                              /**< Scanning Module instance. */
BLE_PB_C_DEF(m_pb_c);                                  /**< Protobuf service client module instance. */
NRF_BLE_QWRS_DEF(m_qwr, NRF_SDH_BLE_TOTAL_LINK_COUNT); /**< Context for the Queued Write module.*/
NRF_BLE_GQ_DEF(m_ble_gatt_queue,                       /**< BLE GATT Queue instance. */
               NRF_SDH_BLE_CENTRAL_LINK_COUNT,
               NRF_BLE_GQ_QUEUE_SIZE);

static bool m_memory_access_in_progress = false; /**< Flag to keep track of the ongoing operations on persistent memory. */
static bool m_scan_on_disconnect_enabled = true;

static ble_central_init_t m_config;

static raw_susbcribe_handler_t m_raw_evt_handler = NULL;

/**< Scan parameters requested for scanning and connection. */
static ble_gap_scan_params_t const m_scan_param =
    {
        .active = 0x01,
        .interval = NRF_BLE_SCAN_SCAN_INTERVAL,
        .window = NRF_BLE_SCAN_SCAN_WINDOW,
        .filter_policy = BLE_GAP_SCAN_FP_ACCEPT_ALL,
        .timeout = NRF_BLE_SCAN_SCAN_DURATION,
        .scan_phys = BLE_GAP_PHY_CODED,
        .extended = 1,
};

/**@brief Function for handling the Heart Rate Service Client and Battery Service Client errors.
 *
 * @param[in]   nrf_error   Error code containing information about what went wrong.
 */
static void service_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}

/**@brief Function for handling database discovery events.
 *
 * @details This function is a callback function to handle events from the database discovery module.
 *          Depending on the UUIDs that are discovered, this function forwards the events
 *          to their respective services.
 *
 * @param[in] p_event  Pointer to the database discovery event.
 */
static void db_disc_handler(ble_db_discovery_evt_t *p_evt)
{
    ble_pb_on_db_disc_evt(&m_pb_c, p_evt);
}

/**@brief Function for handling Heart Rate Collector events.
 *
 * @param[in] p_pb_c       Pointer to Heart Rate Client structure.
 * @param[in] p_evt        Pointer to event structure.
 * @param[in] p_pb_evt     Pointer to event data.
 */
static void pb_c_evt_handler(ble_pb_c_t *p_pb_c, ble_pb_c_evt_t *p_evt)
{
    ret_code_t err_code;

    switch (p_evt->evt_type)
    {
    case BLE_PB_C_EVT_DISCOVERY_COMPLETE:
        err_code = ble_pb_c_handles_assign(p_pb_c,
                                           p_evt->conn_handle,
                                           &p_evt->params.peer_db);
        APP_ERROR_CHECK(err_code);

        // Initiate bonding.
        err_code = pm_conn_secure(p_evt->conn_handle, false);
        if (err_code != NRF_ERROR_BUSY)
        {
            APP_ERROR_CHECK(err_code);
        }

        NRF_LOG_INFO("Protobuf Service discovered ");

        // Enable notifications
        err_code = ble_pb_c_notif_enable(p_pb_c, p_evt->conn_handle);
        APP_ERROR_CHECK(err_code);

        // Continue scan if not full yet.
        if ((ble_conn_state_central_conn_count() < m_config.device_count) &&
            (ble_conn_state_central_conn_count() < NRF_SDH_BLE_CENTRAL_LINK_COUNT))
        {
            ble_central_scan_start();
        }

        break;

    case BLE_PB_C_EVT_NOTIFICATION:

        // Forward to raw handler.
        if (m_raw_evt_handler != NULL)
        {
            m_raw_evt_handler(&(p_evt->params.data));
        }

        break;

    default:
        break;
    }
}

/**@brief Function for assigning new connection handle to available instance of QWR module.
 *
 * @param[in] conn_handle New connection handle.
 */
static void multi_qwr_conn_handle_assign(uint16_t conn_handle)
{
    for (uint32_t i = 0; i < NRF_SDH_BLE_TOTAL_LINK_COUNT; i++)
    {
        if (m_qwr[i].conn_handle == BLE_CONN_HANDLE_INVALID)
        {
            ret_code_t err_code = nrf_ble_qwr_conn_handle_assign(&m_qwr[i], conn_handle);
            APP_ERROR_CHECK(err_code);
            break;
        }
    }
}

static void scan_evt_handler(scan_evt_t const *p_scan_evt)
{
    switch (p_scan_evt->scan_evt_id)
    {
    case NRF_BLE_SCAN_EVT_SCAN_TIMEOUT:
        NRF_LOG_DEBUG("Scan timed out.");
        ble_central_scan_start();

        break;

    default:
        break;
    }
}

/**
 * @brief Function for initializing the Heart Rate Collector.
 */
static void pb_c_init(void)
{
    ble_pb_c_init_t pb_c_init_obj;

    pb_c_init_obj.evt_handler = pb_c_evt_handler;
    pb_c_init_obj.error_handler = service_error_handler;
    pb_c_init_obj.p_gatt_queue = &m_ble_gatt_queue;

    ret_code_t err_code = ble_pb_c_init(&m_pb_c, &pb_c_init_obj);
    APP_ERROR_CHECK(err_code);
}

/**
 * @brief Function for initializing the Database Discovery Collector.
 */
static void db_discovery_init(void)
{
    ble_db_discovery_init_t db_init;

    memset(&db_init, 0, sizeof(ble_db_discovery_init_t));

    db_init.evt_handler = db_disc_handler;
    db_init.p_gatt_queue = &m_ble_gatt_queue;

    ret_code_t err_code = ble_db_discovery_init(&db_init);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for handling the advertising report BLE event.
 *
 * @param[in] p_adv_report  Advertising report from the SoftDevice.
 */
static void on_adv_report(ble_gap_evt_adv_report_t const *p_adv_report)
{
    // uint32_t err_code;
    uint8_t *p_adv_data;
    uint16_t data_len;
    uint16_t field_len;
    uint16_t dev_name_offset = 0;
    char dev_name[DEV_NAME_LEN];

    // Initialize advertisement report for parsing.
    p_adv_data = (uint8_t *)p_adv_report->data.p_data;
    data_len = p_adv_report->data.len;

    // Search for advertising names.
    field_len = ble_advdata_search(p_adv_data,
                                   data_len,
                                   &dev_name_offset,
                                   BLE_GAP_AD_TYPE_COMPLETE_LOCAL_NAME);
    if (field_len == 0)
    {
        // Look for the short local name if it was not found as complete.
        field_len = ble_advdata_search(p_adv_data,
                                       data_len,
                                       &dev_name_offset,
                                       BLE_GAP_AD_TYPE_SHORT_LOCAL_NAME);
        if (field_len == 0)
        {
            // Exit if the data cannot be parsed.
            return;
        }
    }

    memcpy(dev_name, &p_adv_data[dev_name_offset], field_len);
    dev_name[field_len] = 0;

    NRF_LOG_INFO("Found advertising device: %s", nrf_log_push((char *)dev_name));
    NRF_LOG_HEXDUMP_INFO(p_adv_report->peer_addr.addr, BLE_GAP_ADDR_LEN);

    // TODO: Connect to the peer
    // err_code = sd_ble_gap_connect(&p_adv_report->peer_addr,
    //                               &m_scan.scan_params,
    //                               &m_scan.conn_params,
    //                               APP_BLE_CONN_CFG_TAG);
    // APP_ERROR_CHECK(err_code);
}

/**@brief Function for initializing scanning.
 */
static void scan_init(void)
{
    ret_code_t err_code;
    nrf_ble_scan_init_t init_scan;

    memset(&init_scan, 0, sizeof(init_scan));

    init_scan.connect_if_match = true;
    init_scan.conn_cfg_tag = APP_BLE_CONN_CFG_TAG;

    err_code = nrf_ble_scan_init(&m_scan, &init_scan, scan_evt_handler);
    APP_ERROR_CHECK(err_code);

    // Iterate through all the available addresses
    // TODO: update this on change
    for (uint8_t i = 0; i < m_config.device_count; i++)
    {
        err_code = nrf_ble_scan_filter_set(&m_scan,
                                           SCAN_ADDR_FILTER,
                                           m_config.devices[i].addr);
        APP_ERROR_CHECK(err_code);
    }

    // TODO: scan by UUID also?
    // err_code = nrf_ble_scan_filter_set(&m_scan, SCAN_UUID_FILTER, &m_nus_uuid);
    // APP_ERROR_CHECK(err_code);

    // Eanble the filters.
    err_code = nrf_ble_scan_filters_enable(&m_scan,
                                           NRF_BLE_SCAN_ALL_FILTER,
                                           false);
    APP_ERROR_CHECK(err_code);
}

void ble_central_evt_handler(ble_evt_t const *p_ble_evt, void *p_context)
{
    ret_code_t err_code;
    ble_gap_evt_t const *p_gap_evt = &p_ble_evt->evt.gap_evt;

    switch (p_ble_evt->header.evt_id)
    {
    // Upon connection, initiate secure bonding.
    case BLE_GAP_EVT_CONNECTED:

        NRF_LOG_INFO("Connected to handle 0x%x", p_gap_evt->conn_handle);

        // TODO: continue scanning..

        err_code = bsp_indication_set(BSP_INDICATE_CONNECTED);
        APP_ERROR_CHECK(err_code);

        // Discover the peer services.
        err_code = ble_db_discovery_start(&m_db_discovery,
                                          p_gap_evt->conn_handle);
        APP_ERROR_CHECK(err_code);

        // Assign connection handle to the QWR module.
        multi_qwr_conn_handle_assign(p_gap_evt->conn_handle);

        break;

    // Upon disconnection, reset the connection handle of the peer that disconnected
    // and invalidate data taken from the NFC tag.
    case BLE_GAP_EVT_DISCONNECTED:

        err_code = bsp_indication_set(BSP_INDICATE_IDLE);
        APP_ERROR_CHECK(err_code);

        NRF_LOG_INFO("Disconnected. conn_handle: 0x%x, reason: 0x%x",
                     p_gap_evt->conn_handle,
                     p_gap_evt->params.disconnected.reason);

        // Restart scanning.
        if (m_scan_on_disconnect_enabled)
            ble_central_scan_start();
        break;

    case BLE_GAP_EVT_ADV_REPORT:
        // TODO: necessary?
        // on_adv_report(&p_gap_evt->params.adv_report);
        UNUSED_VARIABLE(on_adv_report);
        break;

    case BLE_GAP_EVT_TIMEOUT:
        if (p_gap_evt->params.timeout.src == BLE_GAP_TIMEOUT_SRC_CONN)
        {
            NRF_LOG_INFO("Connection Request timed out.");
        }
        break;

    case BLE_GAP_EVT_CONN_PARAM_UPDATE_REQUEST:
        // Accept parameters requested by the the peer.
        err_code = sd_ble_gap_conn_param_update(p_gap_evt->conn_handle,
                                                &p_gap_evt->params.conn_param_update_request.conn_params);
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
    }
    break;

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

    case BLE_GAP_EVT_AUTH_STATUS:
        NRF_LOG_DEBUG("BLE_GAP_EVT_AUTH_STATUS");
        if (p_ble_evt->evt.gap_evt.params.auth_status.auth_status == BLE_GAP_SEC_STATUS_SUCCESS)
        {
            NRF_LOG_DEBUG("Authorization succeeded!");
        }
        else
        {
            NRF_LOG_WARNING("Authorization failed with code: %u!",
                            p_ble_evt->evt.gap_evt.params.auth_status.auth_status);
        }
        break;

    case BLE_GAP_EVT_CONN_SEC_UPDATE:
        NRF_LOG_DEBUG("BLE_GAP_EVT_CONN_SEC_UPDATE");
        break;

    default:
        // No implementation needed.
        break;
    }
}

void ble_central_scan_start(void)
{
    ret_code_t err_code;

    // If there is any pending write to flash, defer scanning until it completes.
    if (nrf_fstorage_is_busy(NULL))
    {
        m_memory_access_in_progress = true;
        return;
    }

    err_code = nrf_ble_scan_params_set(&m_scan, &m_scan_param);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_ble_scan_start(&m_scan);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for handling the system events of the application.
 *
 * @param[in]   sys_evt   System event.
 */
static void soc_evt_handler(uint32_t sys_evt, void *p_context)
{
    switch (sys_evt)
    {
    case NRF_EVT_FLASH_OPERATION_SUCCESS:
        /* fall through */

    case NRF_EVT_FLASH_OPERATION_ERROR:
        if (m_memory_access_in_progress)
        {
            m_memory_access_in_progress = false;
            ble_central_scan_start();
        }
        break;

    default:
        // No implementation needed.
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

static void tx_power_init()
{
    ret_code_t err_code = sd_ble_gap_tx_power_set(BLE_GAP_TX_POWER_ROLE_SCAN_INIT, 0, NRF_BLE_SCAN_TX_POWER);
    APP_ERROR_CHECK(err_code);
}

void ble_central_init(ble_central_init_t *init)
{

    ret_code_t err_code;

    nrf_ble_qwr_init_t qwr_init = {0};

    // Copy configuration over
    m_config = *init;

    // Initialize Queued Write module instances.
    qwr_init.error_handler = nrf_qwr_error_handler;

    for (uint32_t i = 0; i < NRF_SDH_BLE_TOTAL_LINK_COUNT; i++)
    {
        err_code = nrf_ble_qwr_init(&m_qwr[i], &qwr_init);
        APP_ERROR_CHECK(err_code);
    }

    NRF_SDH_SOC_OBSERVER(m_soc_observer, APP_SOC_OBSERVER_PRIO, soc_evt_handler, NULL);

    tx_power_init(); // Set TX power
    db_discovery_init();
    pb_c_init();
    scan_init();
}

void ble_central_write(uint8_t *data, size_t size)
{
    // TODO: best way of handling non connection
    // Write to all connection handles

    for (int i = 0; i < NRF_SDH_BLE_TOTAL_LINK_COUNT; i++)
    {
        uint16_t conn_handle = m_pb_c.conn_handles[i];

        if (conn_handle != BLE_CONN_HANDLE_INVALID)
        {
            ret_code_t err_code = ble_pb_c_write(&m_pb_c, conn_handle, data, size);
            if (err_code == NRF_ERROR_INVALID_STATE)
            {
                NRF_LOG_WARNING("Not connected. Unable to send message.");
            }
            else
            {
                APP_ERROR_CHECK(err_code);
            }
        }
    }
}

void ble_central_attach_raw_handler(raw_susbcribe_handler_t raw_evt_handler)
{
    m_raw_evt_handler = raw_evt_handler;
}

void ble_central_disconnect()
{

    ret_code_t err_code;

    // Disable scanning on disconnect temporarily
    m_scan_on_disconnect_enabled = false;

    // Stop scanning
    nrf_ble_scan_stop();

    // Check all the handles. If one is valid, return true
    for (int i = 0; i < NRF_SDH_BLE_TOTAL_LINK_COUNT; i++)
    {
        uint16_t conn_handle = m_pb_c.conn_handles[i];

        if (conn_handle != BLE_CONN_HANDLE_INVALID)
        {
            err_code = sd_ble_gap_disconnect(conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            if (err_code != NRF_ERROR_INVALID_STATE)
            {
                APP_ERROR_CHECK(err_code);
            }
        }
    }
}

void ble_central_pm_evt_handler(pm_evt_t const *p_evt)
{
    ret_code_t err_code;

    // ret_code_t err_code;
    switch (p_evt->evt_id)
    {
    case PM_EVT_CONN_SEC_FAILED:
        if (p_evt->params.conn_sec_failed.error == PM_CONN_SEC_ERROR_PIN_OR_KEY_MISSING)
        {
            // Rebond if one party has lost its keys.
            err_code = pm_conn_secure(p_evt->conn_handle, true);
            if (err_code != NRF_ERROR_BUSY)
            {
                APP_ERROR_CHECK(err_code);
            }
        }
        break;
    case PM_EVT_PEERS_DELETE_SUCCEEDED:
        m_scan_on_disconnect_enabled = true;
        ble_central_scan_start();
        break;

    default:
        // No implementation needed.
        break;
    }
}

bool ble_central_is_connected(void)
{

    // Check all the handles. If one is valid, return true
    for (int i = 0; i < NRF_SDH_BLE_TOTAL_LINK_COUNT; i++)
    {
        uint16_t conn_handle = m_pb_c.conn_handles[i];

        if (conn_handle != BLE_CONN_HANDLE_INVALID)
        {
            return true;
        }
    }

    return false;
}