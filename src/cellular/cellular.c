#include "FreeRTOS.h"
#include "task.h"

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

#include "nrf_gpio.h"
#include "nrf_log.h"

// Which UARTE port
#define CELLULAR_UARTE 0
#define CELLULAR_UNUSED -1
#define CELLULAR_BAUD 115200

// Used to check if we're initialized
static bool m_initialized = false;

// UART queue.
static CellularPortQueueHandle_t gQueueHandle = NULL;

uint32_t cellular_init(void)
{

    int32_t errorCode;


    // Get vint input
    nrf_gpio_cfg_input(UB_VINT, NRF_GPIO_PIN_NOPULL);

    // Reset pin
    // nrf_gpio_cfg_output(UB_RST);
    // nrf_gpio_pin_set(UB_RST);

    // CP_ON
    nrf_gpio_cfg_output(UB_PWR_ON);
    nrf_gpio_pin_set(UB_PWR_ON);

    if( nrf_gpio_pin_read(UB_VINT) != 1 ) {
        NRF_LOG_INFO("Power up sequence");
        vTaskDelay(100);
        nrf_gpio_pin_clear(UB_PWR_ON);
        vTaskDelay(300);
        nrf_gpio_pin_set(UB_PWR_ON);
    }

    for (int i = 0; i < 10; i++)
    {
        vTaskDelay(100);

        if (nrf_gpio_pin_read(UB_VINT))
        {
            NRF_LOG_INFO("powered up!");
            break;
        }
    }

    vTaskDelay(5000);

    // Buffer power/enable
    nrf_gpio_cfg_output(UB_BUF_PWR);
    // Note: active low!
    nrf_gpio_pin_clear(UB_BUF_PWR);
    

    // Init function (which does nothing)
    errorCode = cellularPortInit();

    if (errorCode == NRF_SUCCESS)
    {
        errorCode = cellularPortUartInit(UB_TX,
                                         UB_RX,
                                         CELLULAR_UNUSED,
                                         UB_RTS,
                                         CELLULAR_BAUD,
                                         CELLULAR_UNUSED,
                                         CELLULAR_UARTE,
                                         &gQueueHandle);
        if (errorCode == 0)
        {
            errorCode = cellularCtrlInit(CELLULAR_UNUSED,
                                         UB_PWR_ON,
                                         UB_VINT,
                                         false,
                                         CELLULAR_UARTE,
                                         gQueueHandle);
            if (errorCode == 0)
            {
                m_initialized = true;
                NRF_LOG_RAW_INFO("CELLULAR: initialised.\n");
            }
            else
            {
                NRF_LOG_RAW_INFO("CELLULAR: cellularCtrlInit() failed (%d).\n",
                                 errorCode);
            }
        }
        else
        {
            NRF_LOG_RAW_INFO("CELLULAR: cellularPortUartInit() failed (%d).\n",
                             errorCode);
        }
    }
    else
    {
        NRF_LOG_RAW_INFO("CELLULAR: cellularPortInit() failed (%d).\n",
                         errorCode);
    }

    // Power it on!
    if (m_initialized && (cellularCtrlPowerOn(NULL) == 0))
    {
        cellularPortLog("CELLULAR: cellular powered on.\n");
        errorCode = NRF_SUCCESS;
    }

    return errorCode;
}