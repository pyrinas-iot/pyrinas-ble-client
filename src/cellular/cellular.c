#include "FreeRTOS.h"

#include "cellular_cfg_hw.h"
#include "cellular_cfg_module.h"
#include "cellular_cfg_sw.h"
#include "cellular_port.h"
#include "cellular_port_os.h"

#include "cellular_ctrl.h"
#include "cellular_port_clib.h"
#include "cellular_port_debug.h"
#include "cellular_port_uart.h"
#include "cellular_sock.h" // For cellularSockGetHostByName()

#include "boards.h"
#include "cellular.h"

#include "nrf_log.h"

// Used to check if we're initialized
static bool m_initialized = false;

// UART queue.
static CellularPortQueueHandle_t gQueueHandle = NULL;

uint32_t cellular_init(void)
{

    int32_t errorCode;

    // Buffer power/enable
    nrf_gpio_cfg_output(UB_BUF_PWR);
    nrf_gpio_pin_set(UB_BUF_PWR);

    // Init function (which does nothing)
    errorCode = cellularPortInit();

    if (errorCode == NRF_SUCCESS)
    {
        errorCode = cellularPortUartInit(CELLULAR_CFG_PIN_TXD,
                                         CELLULAR_CFG_PIN_RXD,
                                         CELLULAR_CFG_PIN_CTS,
                                         CELLULAR_CFG_PIN_RTS,
                                         CELLULAR_CFG_BAUD_RATE,
                                         CELLULAR_CFG_RTS_THRESHOLD,
                                         CELLULAR_CFG_UART,
                                         &gQueueHandle);
        if (errorCode == 0)
        {
            errorCode = cellularCtrlInit(CELLULAR_CFG_PIN_ENABLE_POWER,
                                         CELLULAR_CFG_PIN_CP_ON,
                                         CELLULAR_CFG_PIN_VINT,
                                         false,
                                         CELLULAR_CFG_UART,
                                         gQueueHandle);
            if (errorCode == 0)
            {
                m_initialized = true;
                NRF_LOG_ERROR("CELLULAR_IOT_WIFI: WIFI_On() initialised.\n");
            }
            else
            {
                NRF_LOG_ERROR("CELLULAR_IOT_WIFI: cellularCtrlInit() failed (%d).\n",
                              errorCode);
            }
        }
        else
        {
            NRF_LOG_ERROR("CELLULAR_IOT_WIFI: cellularPortUartInit() failed (%d).\n",
                          errorCode);
        }
    }
    else
    {
        NRF_LOG_ERROR("CELLULAR_IOT_WIFI: cellularPortInit() failed (%d).\n",
                      errorCode);
    }

    return NRF_SUCCESS;
}