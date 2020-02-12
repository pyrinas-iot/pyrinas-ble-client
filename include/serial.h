#ifndef SERIAL_H
#define SERIAL_H

#include <stdint.h>
#include <stddef.h>

// typedef uint32_t (*serial_begin_t)(uint32_t baud);
// typedef int (*serial_available_t)();
// typedef void (*serial_println_t)(char *data);
// typedef char (*serial_println_t)();

// typedef struct
// {
//   serial_begin_t begin;
//   serial_available_t available;
//   serial_println_t println;
//   serial_read_t read;
// };

void serial_begin(uint32_t baud);
int serial_available();
size_t serial_println(const char *data);
int serial_read();

#endif