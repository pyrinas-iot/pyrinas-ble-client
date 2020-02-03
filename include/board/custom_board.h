#ifndef CUSTOM_BOARD_H
#define CUSTOM_BOARD_H

// LEDs definitions
#define LEDS_NUMBER    1

#define LED_1          25

#define LEDS_ACTIVE_STATE 0

#define LEDS_LIST { LED_1 }

#define BSP_LED_0      LED_1


// Buttons
#define BUTTONS_NUMBER 1

#define BUTTON_START   14
#define BUTTON_1       14
#define BUTTON_STOP    14
#define BUTTON_PULL    NRF_GPIO_PIN_NOPULL

#define BUTTONS_ACTIVE_STATE 0

#define BUTTONS_LIST { BUTTON_ }

#define BSP_BUTTON_0   BUTTON_1


#endif