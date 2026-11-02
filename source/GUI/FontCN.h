#ifndef __FONT_CN_H
#define __FONT_CN_H

#include "Type.h"

#define FONT_CN16_COUNT  130u
#define FONT_CN_SIZE     3u      /* UTF-8: 一个汉字 3 字节 */

extern const uint8_t ROM FontCn16Data[FONT_CN16_COUNT][32];
extern const uint8_t ROM FontCnKey[FONT_CN16_COUNT][3];

#endif
