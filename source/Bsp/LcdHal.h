#ifndef __LCDHAL_H
#define __LCDHAL_H

#include "Type.h"

/*============================================================================
 * LCD 硬件层：只负责"把一页数据搬到屏幕"，不做缓冲、不做延时策略
 *==========================================================================*/

void LcdHal_Init(void);
void LcdHal_WritePage(uint8_t page, const uint8_t XRAM *buf);   /* 写一页 128 字节 */
void LcdHal_SetContrast(uint8_t val);
void LcdHal_SetInvert(uint8_t on);
void LcdHal_DisplayOn(uint8_t on);

#endif /* __LCDHAL_H */
