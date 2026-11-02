#ifndef __GUI_H
#define __GUI_H

#include "Type.h"
#include "Config.h"

/*============================================================================
 * GUI 层：1KB 显存 + 图元 + 中英混排文本 + 脏页刷新任务
 *   绘制只改显存（快），搬屏由 GUI_FlushTask() 分批完成（不阻塞）
 *==========================================================================*/

typedef enum {
    GUI_FONT_8  = 0u,       /* 5x7 ASCII / 16x16 汉字 */
    GUI_FONT_16             /* ASCII 放大 2 倍        */
} GuiFont_t;

typedef enum {
    GUI_MODE_CLR = 0u,      /* 灭 */
    GUI_MODE_SET,           /* 亮 */
    GUI_MODE_XOR            /* 翻转 */
} GuiMode_t;

/* 显存：8 页 x 128 字节，bit0 在页内最上面一行 */
extern uint8_t XRAM GUI_VRam[LCD_PAGES][LCD_WIDTH];

/*------------------------------------------------------------ 生命周期 */
void GUI_Init(void);
void GUI_Clear(void);
void GUI_DirtyAll(void);
void GUI_FlushTask(void);               /* 主循环周期调用 */

/*------------------------------------------------------------ 图元 */
void    GUI_SetPixel(uint8_t x, uint8_t y, uint8_t mode);
uint8_t GUI_GetPixel(uint8_t x, uint8_t y);
void    GUI_HLine(uint8_t x0, uint8_t x1, uint8_t y, uint8_t mode);
void    GUI_VLine(uint8_t x, uint8_t y0, uint8_t y1, uint8_t mode);
void    GUI_Line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t mode);
void    GUI_Rect(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t mode);
void    GUI_FillRect(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t mode);
void    GUI_InvertRect(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);
void    GUI_ProgressBar(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t percent);
void    GUI_ScrollBar(uint8_t x, uint8_t y, uint8_t h, uint8_t total, uint8_t view, uint8_t top);

/*------------------------------------------------------------ 文本 */
uint8_t GUI_TextWidth(const char ROM *s, GuiFont_t f);
uint8_t GUI_DrawText(uint8_t x, uint8_t y, const char ROM *s, GuiFont_t f);
uint8_t GUI_DrawTextInv(uint8_t x, uint8_t y, const char ROM *s, GuiFont_t f);
uint8_t GUI_DrawTextCenter(uint8_t y, const char ROM *s, GuiFont_t f);

/* 运行时(RAM)字符串版本：参数值、时间、版本号等 */
uint8_t GUI_TextWidthRam(const char *s, GuiFont_t f);
uint8_t GUI_DrawTextRam(uint8_t x, uint8_t y, const char *s, GuiFont_t f);
uint8_t GUI_DrawTextCenterRam(uint8_t y, const char *s, GuiFont_t f);

uint8_t GUI_FontHeight(GuiFont_t f);

#endif /* __GUI_H */
