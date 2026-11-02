#ifndef __FONT_ASCII_H
#define __FONT_ASCII_H

#include "Type.h"

#define FONT_ASCII_FIRST  0x20u
#define FONT_ASCII_LAST   0x7Eu
#define FONT_ASCII_COUNT  (FONT_ASCII_LAST - FONT_ASCII_FIRST + 1u)
#define FONT_ASCII_W      5u      /* 点阵宽 */
#define FONT_ASCII_H      7u      /* 点阵高 */

extern const uint8_t ROM FontAscii5x7[FONT_ASCII_COUNT][7];

#endif
