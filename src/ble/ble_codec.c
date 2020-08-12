#include "ble_codec.h"
#include "cbor.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"

int ble_codec_encode(const ble_event_t *p_data, uint8_t *p_buf, size_t buf_len, size_t *p_size)
{
    CborEncoder cbor, cbor_map;
    CborError err;
    int i = 0;

    cbor_encoder_init(&cbor, p_buf, buf_len, 0);

    err = cbor_encoder_create_map(&cbor, &cbor_map, CborIndefiniteLength);
    if (err) return -1;

    cbor_encode_uint(&cbor_map, i++);
    err = cbor_encode_byte_string(&cbor_map, p_data->name.bytes, p_data->name.size);
    if (err) return -1;

    cbor_encode_uint(&cbor_map, i++);
    err = cbor_encode_byte_string(&cbor_map, p_data->data.bytes, p_data->data.size);
    if (err) return -1;

    cbor_encode_uint(&cbor_map, i++);
    err = cbor_encode_byte_string(&cbor_map, p_data->faddr, sizeof(p_data->faddr));
    if (err) return -1;

    cbor_encode_uint(&cbor_map, i++);
    err = cbor_encode_byte_string(&cbor_map, p_data->taddr, sizeof(p_data->taddr));
    if (err) return -1;

    err = cbor_encoder_close_container(&cbor, &cbor_map);
    if (err) return -1;

    *p_size = cbor_encoder_get_buffer_size(&cbor, p_buf);

    NRF_LOG_HEXDUMP_DEBUG(p_buf,*p_size);

    return 0;
}

int ble_codec_decode(ble_event_t *p_data, const uint8_t *p_buf, size_t len)
{

    CborParser parser;
    CborValue value, map_value;
    CborError err;

    err = cbor_parser_init(p_buf, len, 0, &parser, &value);
    if (err) return -1;

    NRF_LOG_HEXDUMP_DEBUG(p_buf,len);

    /* Return if we're not dealing with a map*/
    if (!cbor_value_is_map(&value))
    {
        NRF_LOG_ERROR("Unexpected CBOR data structure.\n");
        return -1;
    }

    /* Enter map */
    cbor_value_enter_container(&value, &map_value);

    /* Get name */
    size_t size = sizeof(p_data->name.bytes);
    cbor_value_advance_fixed(&map_value);
    err = cbor_value_copy_byte_string(&map_value, p_data->name.bytes, &size, &map_value);
    if (err) return -1;
    p_data->name.size = size;

    // Get data
    size = sizeof(p_data->data.bytes);
    cbor_value_advance_fixed(&map_value);
    err = cbor_value_copy_byte_string(&map_value, p_data->data.bytes, &size, &map_value);
    if (err) return -1;
    p_data->data.size = size;

    size = sizeof(p_data->faddr);
    cbor_value_advance_fixed(&map_value);
    err = cbor_value_copy_byte_string(&map_value, p_data->faddr, &size, &map_value);
    if (err) return -1;

    size = sizeof(p_data->taddr);
    cbor_value_advance_fixed(&map_value);
    err = cbor_value_copy_byte_string(&map_value, p_data->taddr, &size, &map_value);
    if (err) return -1;

    return 0;
}