#ifndef SETUP_H
#define SETUP_H

#define BLE_PB_ENABLED true
#define BLE_PB_CONFIG_LOG_ENABLED true
#define BLE_PB_BLE_OBSERVER_PRIO 2

// PB Collector
#define BLE_PB_C_ENABLED true
#define BLE_PB_C_CONFIG_LOG_ENABLED true

// Log Config
#define BLE_PB_CONFIG_LOG_LEVEL 3
#define BLE_PB_CONFIG_INFO_COLOR 0
#define BLE_PB_CONFIG_DEBUG_COLOR 3

// Security Configuration
#define SEC_PARAM_BOND 1                               /**< Perform bonding. */
#define SEC_PARAM_MITM 0                               /**< Man In The Middle protection not required. */
#define SEC_PARAM_LESC 0                               /**< LE Secure Connections not enabled. */
#define SEC_PARAM_KEYPRESS 0                           /**< Keypress notifications not enabled. */
#define SEC_PARAM_IO_CAPABILITIES BLE_GAP_IO_CAPS_NONE /**< No I/O capabilities. */
#define SEC_PARAM_OOB 0                                /**< Out Of Band data not available. */
#define SEC_PARAM_MIN_KEY_SIZE 7                       /**< Minimum encryption key size. */
#define SEC_PARAM_MAX_KEY_SIZE 16                      /**< Maximum encryption key size. */

// Gatt Queue related
#define NRF_BLE_GQ_QUEUE_SIZE 4
#define NRF_BLE_GQ_DATAPOOL_ELEMENT_SIZE 20

#endif //SETUP_H