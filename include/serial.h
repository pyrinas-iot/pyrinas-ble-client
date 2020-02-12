#ifndef SERIAL_H
#define SERIAL_H

#include <stdint.h>
#include <stddef.h>

void serial_begin(uint32_t baud);
int serial_available(void);
size_t serial_println(const char *data);
int serial_read(void);
void serial_process(void);

#endif