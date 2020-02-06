#include "app_timer.h"
#include "bsp.h"
#include "util.h"

#include "ble_advdata.h"
#include "ble_central.h"
#include "ble_conn_params.h"
#include "ble_db_discovery.h"
#include "ble_m.h"
#include "ble_pb_c.h"

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

BLE_DB_DISCOVERY_DEF(m_db_discovery); /**< Database Discovery module instance. */
NRF_BLE_SCAN_DEF(m_scan);             /**< Scanning Module instance. */
BLE_PB_C_DEF(m_pb_c);                 /**< Protobuf service client module instance. */
NRF_BLE_GQ_DEF(m_ble_gatt_queue,      /**< BLE GATT Queue instance. */
               NRF_SDH_BLE_CENTRAL_LINK_COUNT,
               NRF_BLE_GQ_QUEUE_SIZE);

static bool m_is_connected = false;                      /**< Flag to keep track of the BLE connections with peripheral devices. */
static bool m_memory_access_in_progress = false;         /**< Flag to keep track of the ongoing operations on persistent memory. */
static uint16_t m_conn_handle = BLE_CONN_HANDLE_INVALID; /**< Current connection handle. */
static bool m_pb_notif_enabled = false;                  /**< Flag indicating that pb notification has been enabled. */
static ble_central_init_t m_config;

/**< Scan parameters requested for scanning and connection. */
// static ble_gap_scan_params_t const m_scan_param =
//     {
//         .active = 0x01,
//         .interval = NRF_BLE_SCAN_SCAN_INTERVAL,
//         .window = NRF_BLE_SCAN_SCAN_WINDOW,
//         .filter_policy = BLE_GAP_SCAN_FP_WHITELIST,
//         .timeout = SCAN_DURATION_WITELIST,
//         .scan_phys = BLE_GAP_PHY_AUTO,
// };

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
static void pb_c_evt_handler(ble_pb_c_t *p_pb_c, ble_pb_c_evt_t *p_evt, protobuf_event_t *p_pb_evt)
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
            err_code = pm_conn_secure(p_pb_c->conn_handle, false);
            if (err_code != NRF_ERROR_BUSY)
            {
                APP_ERROR_CHECK(err_code);
            }

            NRF_LOG_INFO("Protobuf Service discovered ");

            // Enable notifications
            err_code = ble_pb_c_notif_enable(p_pb_c);
            APP_ERROR_CHECK(err_code);

            ble_publish("test", "event");

            break;

        case BLE_PB_C_EVT_NOTIFICATION:
            // TODO: forward this on to the main context
            NRF_LOG_HEXDUMP_INFO(p_pb_evt->name.bytes, p_pb_evt->name.size)
            NRF_LOG_HEXDUMP_INFO(p_pb_evt->data.bytes, p_pb_evt->data.size)
            m_pb_notif_enabled = true;
            break;

        default:
            break;
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

    // init_scan.p_scan_param = &m_scan_param;
    init_scan.connect_if_match = true;
    init_scan.conn_cfg_tag = APP_BLE_CONN_CFG_TAG;

    err_code = nrf_ble_scan_init(&m_scan, &init_scan, scan_evt_handler);
    APP_ERROR_CHECK(err_code);

    // Iterate through all the available addresses
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

            NRF_LOG_INFO("Connected.");

            // TODO: continue scanning..

            err_code = bsp_indication_set(BSP_INDICATE_CONNECTED);
            APP_ERROR_CHECK(err_code);

            // Discover the peer services.
            err_code = ble_db_discovery_start(&m_db_discovery,
                                              p_ble_evt->evt.gap_evt.conn_handle);
            APP_ERROR_CHECK(err_code);

            m_is_connected = true;
            m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
            break;

        // Upon disconnection, reset the connection handle of the peer that disconnected
        // and invalidate data taken from the NFC tag.
        case BLE_GAP_EVT_DISCONNECTED:

            err_code = bsp_indication_set(BSP_INDICATE_IDLE);
            APP_ERROR_CHECK(err_code);

            NRF_LOG_INFO("Disconnected. conn_handle: 0x%x, reason: 0x%x",
                         p_gap_evt->conn_handle,
                         p_gap_evt->params.disconnected.reason);

            m_conn_handle = BLE_CONN_HANDLE_INVALID;
            m_is_connected = false;
            m_pb_notif_enabled = false;

            // Restart scanning.
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

/**@brief Function for handling a Connection Parameters error.
 *
 * @param[in] nrf_error  Error code containing information about what went wrong.
 */
static void conn_params_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}

/**@brief Function for initializing the Connection Parameters module. */
static void conn_params_init(void)
{
    ret_code_t err_code;
    ble_conn_params_init_t cp_init;

    memset(&cp_init, 0, sizeof(cp_init));

    cp_init.p_conn_params = NULL;
    cp_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
    cp_init.next_conn_params_update_delay = NEXT_CONN_PARAMS_UPDATE_DELAY;
    cp_init.max_conn_params_update_count = MAX_CONN_PARAMS_UPDATE_COUNT;
    cp_init.start_on_notify_cccd_handle = BLE_GATT_HANDLE_INVALID; // Start upon connection.
    cp_init.disconnect_on_fail = true;
    cp_init.evt_handler = NULL; // Ignore events.
    cp_init.error_handler = conn_params_error_handler;

    err_code = ble_conn_params_init(&cp_init);
    APP_ERROR_CHECK(err_code);
}

void ble_central_init(ble_central_init_t *init)
{
    // Copy configuration over
    m_config = *init;

    NRF_SDH_SOC_OBSERVER(m_soc_observer, APP_SOC_OBSERVER_PRIO, soc_evt_handler, NULL);

    db_discovery_init();
    pb_c_init();
    scan_init();
    conn_params_init();
    // UNUSED_VARIABLE(conn_params_init);
}

void ble_central_write(uint8_t *data, size_t size)
{
    ret_code_t err_code = ble_pb_c_write(&m_pb_c, data, size);
    APP_ERROR_CHECK(err_code);
}