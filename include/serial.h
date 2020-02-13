#ifndef SERIAL_H
#define SERIAL_H

#include <stddef.h>
#include <stdint.h>

void serial_begin(uint32_t baud);
void serial_begin_pins(uint32_t baud, uint8_t tx, uint8_t rx);
int serial_available(void);
size_t serial_println(const char *data);
int serial_read(void);
void serial_process(void);

#endif