#ifndef UTIL_H
#define UTIL_H

#include <stdint.h>

#define member_size(type, member) sizeof(((type *)0)->member)

#define STRX(a) #a
#define STR(a) STRX(a)

void addr_strhex_delim(uint8_t *addr, int size, char *result);

#endif