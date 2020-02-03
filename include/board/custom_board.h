#ifndef CUSTOM_BOARD_H
#define CUSTOM_BOARD_H

#define XENON  0

#ifndef BOARD_VARIANT
#define BOARD_VARIANT XENON
#endif

#if BOARD_VARIANT == XENON
#include "xenon.h"
#else
#error BOARD_VARIANT is not defined!
#endif

#endif