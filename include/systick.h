#ifndef SYSTICK_H
#define SYSTICK_H

#include <stdint.h>

typedef uint32_t systick_ticks_t;

systick_ticks_t systick_get_ticks(void);
systick_ticks_t systick_get_diff_now(systick_ticks_t begin);

#endif