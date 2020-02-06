#include "util.h"

void bin_to_strhex_delim(const uint8_t *data, int size, char delim, char *result)
{
    static char hex_str[] = "0123456789abcdef";
    unsigned int i;

    int factor = (delim == 0) ? 2 : 3;

    for (i = 0; i < size; i++)
    {
        (result)[i * factor + 0] = hex_str[(data[i] >> 4) & 0x0F];
        (result)[i * factor + 1] = hex_str[(data[i]) & 0x0F];

        if (delim != 0)
        {
            (result)[i * factor + 2] = delim;
        }
    }
    (result)[(i - 1) * factor + 2] = '\0';
}