#include "util.h"

void addr_strhex_delim(uint8_t *addr, int size, char *out)
{
    static char hex_str[] = "0123456789abcdef";
    unsigned int i;

    int factor = 3;

    for (i = 0; i < size; i++)
    {
        (out)[i * factor + 0] = hex_str[(addr[i] >> 4) & 0x0F];
        (out)[i * factor + 1] = hex_str[(addr[i]) & 0x0F];
        (out)[i * factor + 2] = ':';
    }
    (out)[(i - 1) * factor + 2] = '\0';
}
