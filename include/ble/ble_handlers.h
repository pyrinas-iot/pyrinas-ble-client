#ifndef BLE_HANDLERS_H
#define BLE_HANDLERS_H

#include "command.pb.h"

// TODO: duplicate handlers that are doing the same thing as Central and Peripheral
/**@brief Subscription handler definition. */
typedef void (*susbcribe_handler_t)(char *name, char *data);

/**@brief Raw subscription handler definition. */
typedef void (*raw_susbcribe_handler_t)(protobuf_event_t *evt);

#endif