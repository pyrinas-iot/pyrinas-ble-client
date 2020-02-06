#ifndef UTIL_H
#define UTIL_H

#include <stdint.h>

#define member_size(type, member) sizeof(((type *)0)->member)

void bin_to_strhex_delim(const uint8_t *data, int size, char delim, char *result);

#endif