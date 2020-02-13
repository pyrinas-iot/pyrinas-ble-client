#ifndef PINS_H
#define PINS_H

#include <nrfx.h>
#include <stdbool.h>
#include <stdint.h>

#include "nrf_gpio.h"

__STATIC_INLINE void pin_config_output(uint8_t pin_number);
__STATIC_INLINE void pin_write(uint8_t pin_number, bool value);

__STATIC_INLINE void pin_config_output(uint8_t pin_number)
{
    nrf_gpio_cfg_output(pin_number);
}

__STATIC_INLINE void pin_write(uint8_t pin_number, bool value)
{
    nrf_gpio_pin_write(pin_number, value);
}

#endif