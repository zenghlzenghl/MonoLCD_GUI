#ifndef __FONT_H
#define __FONT_H

#include "Type.h"
#include "GUI/FontAscii.h"
#include "GUI/FontCN.h"

/*============================================================================
 * 字库：ASCII 5x7（可 2 倍放大）+ 汉字 16x16
 *   FontAscii.c / FontCN.c 由 Tools/gen_font.py 生成
 *==========================================================================*/

#define FONT_CN_W           16u
#define FONT_CN_H           16u
#define FONT_CN_NOT_FOUND   0xFFu

/* 按 UTF-8 三个字节二分查找汉字，返回字模索引；未收录返回 FONT_CN_NOT_FOUND
 * 传字节而不传指针：ROM/RAM 两种字符串都能用 */
uint8_t            FontCnFind(uint8_t b0, uint8_t b1, uint8_t b2);
const uint8_t ROM *FontCnData(uint8_t idx);

#endif /* __FONT_H */
