#ifndef TIMER_H
#define TIMER_H

#include "app_timer.h"

#define timer_define(_id) _APP_TIMER_DEF(_id)

/**@brief Timer modes. */
typedef enum
{
    TIMER_SINGLE_SHOT = APP_TIMER_MODE_SINGLE_SHOT, /**< The timer will expire only once. */
    TIMER_REPEATED = APP_TIMER_MODE_REPEATED        /**< The timer will restart each time it expires. */
} timer_mode_t;

typedef void (*timer_evt_t)();

void timer_create(const app_timer_id_t *p_timer_id, timer_mode_t mode,
                  timer_evt_t timeout_handler);

void timer_start(const app_timer_id_t *p_timer_id, uint32_t timeout_ms);

void timer_stop(const app_timer_id_t *p_timer_id);

#endif