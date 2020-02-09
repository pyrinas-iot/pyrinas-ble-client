/* Automatically generated nanopb header */
/* Generated by nanopb-0.4.1 */

#ifndef PB_COMMAND_PB_H_INCLUDED
#define PB_COMMAND_PB_H_INCLUDED
#include <pb.h>

#if PB_PROTO_HEADER_VERSION != 40
#error Regenerate this file with the current version of nanopb generator.
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Enum definitions */
typedef enum _event_type {
    event_type_command = 0,
    event_type_response = 1
} event_type;

/* Struct definitions */
typedef PB_BYTES_ARRAY_T(12) protobuf_event_t_name_t;
typedef PB_BYTES_ARRAY_T(64) protobuf_event_t_data_t;
typedef struct _protobuf_event_t {
    event_type type;
    protobuf_event_t_name_t name;
    protobuf_event_t_data_t data;
    pb_byte_t addr[6];
} protobuf_event_t;


/* Helper constants for enums */
#define _event_type_MIN event_type_command
#define _event_type_MAX event_type_response
#define _event_type_ARRAYSIZE ((event_type)(event_type_response+1))


/* Initializer values for message structs */
#define protobuf_event_t_init_default            {_event_type_MIN, {0, {0}}, {0, {0}}, {0}}
#define protobuf_event_t_init_zero               {_event_type_MIN, {0, {0}}, {0, {0}}, {0}}

/* Field tags (for use in manual encoding/decoding) */
#define protobuf_event_t_type_tag                1
#define protobuf_event_t_name_tag                2
#define protobuf_event_t_data_tag                3
#define protobuf_event_t_addr_tag                4

/* Struct field encoding specification for nanopb */
#define protobuf_event_t_FIELDLIST(X, a) \
X(a, STATIC,   SINGULAR, UENUM,    type,              1) \
X(a, STATIC,   SINGULAR, BYTES,    name,              2) \
X(a, STATIC,   SINGULAR, BYTES,    data,              3) \
X(a, STATIC,   SINGULAR, FIXED_LENGTH_BYTES, addr,              4)
#define protobuf_event_t_CALLBACK NULL
#define protobuf_event_t_DEFAULT NULL

extern const pb_msgdesc_t protobuf_event_t_msg;

/* Defines for backwards compatibility with code written before nanopb-0.4.0 */
#define protobuf_event_t_fields &protobuf_event_t_msg

/* Maximum encoded size of messages (where known) */
#define protobuf_event_t_size                    90

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
