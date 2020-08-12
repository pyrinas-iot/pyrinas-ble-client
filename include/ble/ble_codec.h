#ifndef _BLE_CODEC_H
#define _BLE_CODEC_H

#include <stddef.h>
#include <stdint.h>

typedef struct
{
    uint8_t size;
    uint8_t bytes[18];
} ble_event_name_data_t;

typedef struct
{
    uint8_t size;
    uint8_t bytes[128];
} ble_event_data_t;

typedef struct
{
    ble_event_name_data_t name;
    ble_event_data_t data;
    uint8_t faddr[6];
    uint8_t taddr[6];
} ble_event_t;

int ble_codec_encode(const ble_event_t *p_data, uint8_t *p_buf, size_t len, size_t *p_size);
int ble_codec_decode(ble_event_t *p_data, const uint8_t *p_buf, size_t len);

#endif