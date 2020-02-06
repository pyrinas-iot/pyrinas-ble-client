#ifndef XENON_H
#define XENON_H

#include "nrf_gpio.h"

// Mode button
#define MODE_BUTTON NRF_GPIO_PIN_MAP(0, 11)

// I2C
#define SDA NRF_GPIO_PIN_MAP(0, 26)
#define SCL NRF_GPIO_PIN_MAP(0, 27)

// UART
#define TX NRF_GPIO_PIN_MAP(0, 6)
#define RX NRF_GPIO_PIN_MAP(0, 8)

// Digital input/output
#define D2 NRF_GPIO_PIN_MAP(1, 1)
#define D3 NRF_GPIO_PIN_MAP(1, 2)
#define D4 NRF_GPIO_PIN_MAP(1, 8)
#define D5 NRF_GPIO_PIN_MAP(1, 10)
#define D6 NRF_GPIO_PIN_MAP(1, 11)
#define D7 NRF_GPIO_PIN_MAP(1, 12)
#define D8 NRF_GPIO_PIN_MAP(1, 3)

// Analog related
#define A0 NRF_GPIO_PIN_MAP(0, 3)
#define A1 NRF_GPIO_PIN_MAP(0, 4)
#define AREF NRF_GPIO_PIN_MAP(0, 2)
#define A2 NRF_GPIO_PIN_MAP(0, 28)
#define A3 NRF_GPIO_PIN_MAP(0, 29)
#define A4 NRF_GPIO_PIN_MAP(0, 30)
#define A5 NRF_GPIO_PIN_MAP(0, 31)

// Onboard LEDs
#define RGB_RED_LED NRF_GPIO_PIN_MAP(0, 13)
#define RGB_GREEN_LED NRF_GPIO_PIN_MAP(0, 14)
#define RGB_BLUE_LED NRF_GPIO_PIN_MAP(0, 15)
#define BLUE_LED D7

// SPI related
#define MOSI NRF_GPIO_PIN_MAP(1, 13)
#define MISO NRF_GPIO_PIN_MAP(1, 14)
#define SCK NRF_GPIO_PIN_MAP(1, 15)

// External Flash
#define F_CS NRF_GPIO_PIN_MAP(0, 17)
#define F_CLK NRF_GPIO_PIN_MAP(0, 19)
#define F_SI NRF_GPIO_PIN_MAP(0, 20)
#define F_SO NRF_GPIO_PIN_MAP(0, 21)
#define F_WP NRF_GPIO_PIN_MAP(0, 22)
#define F_HOLD NRF_GPIO_PIN_MAP(0, 23)

// Antenna Control
#define VCTL2 NRF_GPIO_PIN_MAP(0, 25)
#define VCTL1 NRF_GPIO_PIN_MAP(0, 24)

// Battery related
#define CHG NRF_GPIO_PIN_MAP(1, 9)
#define BAT_DET NRF_GPIO_PIN_MAP(0, 5)

// LEDs definitions
#define LEDS_NUMBER 1
#define LEDS_ACTIVE_STATE 0
#define LEDS_LIST     \
    {                 \
        RGB_GREEN_LED \
    }
#define BSP_LED_0 RGB_GREEN_LED

// Buttons
#define BUTTONS_NUMBER 1

#define BUTTON_START MODE_BUTTON
#define BUTTON_1 MODE_BUTTON
#define BUTTON_STOP MODE_BUTTON
#define BUTTON_PULL NRF_GPIO_PIN_PULLUP

#define BUTTONS_ACTIVE_STATE 0
#define BUTTONS_LIST \
    {                \
        BUTTON_1     \
    }
#define BSP_BUTTON_0 BUTTON_1

#endif