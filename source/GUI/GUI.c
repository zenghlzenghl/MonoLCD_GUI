#include "GUI/GUI.h"
#include "GUI/Font.h"
#include "Bsp/LcdHal.h"
#include "Bsp/Timer.h"
#include "Config.h"

#pragma disable_warning 110     /* SDCC：循环条件被优化器改写，属正常提示 */

/*============================================================================
 * GUI 实现
 *==========================================================================*/

#define ASCII_ADV       6u      /* ASCII 字符步进：5 列 + 1 列间距 */
#define ASCII_GLYPH_H   7u      /* 5x7 字模行数 */
#define ASCII_LINE_H    8u      /* 小字行高 */
#define CN_ADV          16u

uint8_t XRAM GUI_VRam[LCD_PAGES][LCD_WIDTH];

static uint8_t  s_dirtyMask;
static uint16_t s_flushTick;

/*------------------------------------------------------------ 内部工具 */
static void GUI_MarkDirty(uint8_t y0, uint8_t y1)
{
    uint8_t p0 = (uint8_t)(y0 >> 3);
    uint8_t p1 = (uint8_t)(y1 >> 3);
    uint8_t p;

    if (y1 >= LCD_HEIGHT) {
        y1 = (uint8_t)(LCD_HEIGHT - 1u);
    }
    if (p1 >= (uint8_t)LCD_PAGES) {
        p1 = (uint8_t)(LCD_PAGES - 1u);
    }
    for (p = p0; p <= p1; p++) {
        s_dirtyMask |= (uint8_t)(1u << p);
    }
}

/*------------------------------------------------------------ 基本图元 */
void GUI_SetPixel(uint8_t x, uint8_t y, uint8_t mode)
{
    uint8_t page;
    uint8_t bit;
    uint8_t v;

    if ((x >= LCD_WIDTH) || (y >= LCD_HEIGHT)) {
        return;
    }
    page = (uint8_t)(y >> 3);
    bit  = (uint8_t)(1u << (y & 0x07u));
    v    = GUI_VRam[page][x];

    switch (mode) {
    case GUI_MODE_SET: v |= bit;  break;
    case GUI_MODE_CLR: v &= (uint8_t)~bit; break;
    case GUI_MODE_XOR: v ^= bit;  break;
    default: break;
    }
    GUI_VRam[page][x] = v;
    GUI_MarkDirty(y, y);
}

uint8_t GUI_GetPixel(uint8_t x, uint8_t y)
{
    if ((x >= LCD_WIDTH) || (y >= LCD_HEIGHT)) {
        return 0u;
    }
    return (GUI_VRam[(uint8_t)(y >> 3)][x] & (uint8_t)(1u << (y & 0x07u))) ? 1u : 0u;
}

void GUI_HLine(uint8_t x0, uint8_t x1, uint8_t y, uint8_t mode)
{
    uint8_t x;

    if (y >= LCD_HEIGHT) {
        return;
    }
    if (x0 > x1) {
        x = x0; x0 = x1; x1 = x;
    }
    if (x1 >= LCD_WIDTH) {
        x1 = (uint8_t)(LCD_WIDTH - 1u);
    }
    for (x = x0;; x++) {
        GUI_SetPixel(x, y, mode);
        if (x >= x1) {
            break;
        }
    }
}

void GUI_VLine(uint8_t x, uint8_t y0, uint8_t y1, uint8_t mode)
{
    uint8_t y;

    if (x >= LCD_WIDTH) {
        return;
    }
    if (y0 > y1) {
        y = y0; y0 = y1; y1 = y;
    }
    if (y1 >= LCD_HEIGHT) {
        y1 = (uint8_t)(LCD_HEIGHT - 1u);
    }
    for (y = y0;; y++) {
        GUI_SetPixel(x, y, mode);
        if (y >= y1) {
            break;
        }
    }
}

void GUI_Line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t mode)
{
    int16_t dx = (x1 > x0) ? (int16_t)(x1 - x0) : (int16_t)(x0 - x1);
    int16_t dy = (y1 > y0) ? (int16_t)(y1 - y0) : (int16_t)(y0 - y1);
    int16_t sx = (x0 < x1) ? 1 : -1;
    int16_t sy = (y0 < y1) ? 1 : -1;
    int16_t err = (int16_t)(dx - dy);

    while (1) {
        if ((x0 >= 0) && (y0 >= 0) && (x0 < (int16_t)LCD_WIDTH) && (y0 < (int16_t)LCD_HEIGHT)) {
            GUI_SetPixel((uint8_t)x0, (uint8_t)y0, mode);
        }
        if ((x0 == x1) && (y0 == y1)) {
            break;
        }
        {
            int16_t e2 = (int16_t)(err << 1);
            if (e2 > -dy) { err -= dy; x0 += sx; }
            if (e2 <  dx) { err += dx; y0 += sy; }
        }
    }
}

void GUI_Rect(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t mode)
{
    GUI_HLine(x0, x1, y0, mode);
    GUI_HLine(x0, x1, y1, mode);
    GUI_VLine(x0, y0, y1, mode);
    GUI_VLine(x1, y0, y1, mode);
}

void GUI_FillRect(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t mode)
{
    uint8_t x;
    uint8_t y;

    if (x0 > x1) { x = x0; x0 = x1; x1 = x; }
    if (y0 > y1) { y = y0; y0 = y1; y1 = y; }
    if (x1 >= LCD_WIDTH)  { x1 = (uint8_t)(LCD_WIDTH - 1u); }
    if (y1 >= LCD_HEIGHT) { y1 = (uint8_t)(LCD_HEIGHT - 1u); }

    for (y = y0;; y++) {
        for (x = x0;; x++) {
            GUI_SetPixel(x, y, mode);
            if (x >= x1) {
                break;
            }
        }
        if (y >= y1) {
            break;
        }
    }
}

void GUI_InvertRect(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
    uint8_t x;
    uint8_t y;

    if (x0 > x1) { x = x0; x0 = x1; x1 = x; }
    if (y0 > y1) { y = y0; y0 = y1; y1 = y; }
    if (x1 >= LCD_WIDTH)  { x1 = (uint8_t)(LCD_WIDTH - 1u); }
    if (y1 >= LCD_HEIGHT) { y1 = (uint8_t)(LCD_HEIGHT - 1u); }

    for (y = y0;; y++) {
        for (x = x0;; x++) {
            GUI_SetPixel(x, y, GUI_MODE_XOR);
            if (x >= x1) {
                break;
            }
        }
        if (y >= y1) {
            break;
        }
    }
}

void GUI_ProgressBar(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t percent)
{
    uint8_t x1 = (uint8_t)(x + w - 1u);
    uint8_t y1 = (uint8_t)(y + h - 1u);
    uint8_t fill;

    if (percent > 100u) {
        percent = 100u;
    }
    GUI_Rect(x, y, x1, y1, GUI_MODE_SET);
    fill = (uint8_t)((uint16_t)((uint16_t)(w - 2u) * percent) / 100u);
    if (fill != 0u) {
        GUI_FillRect((uint8_t)(x + 1u), (uint8_t)(y + 1u),
                     (uint8_t)(x + fill), (uint8_t)(y1 - 1u), GUI_MODE_SET);
    }
}

void GUI_ScrollBar(uint8_t x, uint8_t y, uint8_t h, uint8_t total, uint8_t view, uint8_t top)
{
    uint8_t y1 = (uint8_t)(y + h - 1u);
    uint8_t barH;
    uint8_t barY;

    if ((total == 0u) || (view >= total)) {
        return;
    }
    barH = (uint8_t)((uint16_t)(h * view) / total);
    if (barH == 0u) {
        barH = 1u;
    }
    barY = (uint8_t)(y + (uint8_t)((uint16_t)((h - barH) * top) / (total - view)));
    GUI_VLine(x, y, y1, GUI_MODE_SET);
    GUI_FillRect((uint8_t)(x - 1u), barY, (uint8_t)(x + 1u), (uint8_t)(barY + barH - 1u), GUI_MODE_SET);
}

/*------------------------------------------------------------ 文本 */
static void GUI_DrawAscii(uint8_t x, uint8_t y, uint8_t c, GuiFont_t f)
{
    const uint8_t ROM *p;
    uint8_t i;
    uint8_t j;

    if ((c < FONT_ASCII_FIRST) || (c > FONT_ASCII_LAST)) {
        return;
    }
    p = &FontAscii5x7[c - FONT_ASCII_FIRST][0u];

    if (f == GUI_FONT_16) {
        for (j = 0u; j < ASCII_GLYPH_H; j++) {
            for (i = 0u; i < FONT_ASCII_W; i++) {
                if ((p[j] & (uint8_t)(0x80u >> i)) != 0u) {
                    GUI_FillRect((uint8_t)(x + (uint8_t)(i * 2u)),
                                 (uint8_t)(y + 1u + (uint8_t)(j * 2u)),
                                 (uint8_t)(x + (uint8_t)(i * 2u) + 1u),
                                 (uint8_t)(y + 1u + (uint8_t)(j * 2u) + 1u), GUI_MODE_SET);
                }
            }
        }
    } else {
        for (j = 0u; j < ASCII_GLYPH_H; j++) {
            for (i = 0u; i < FONT_ASCII_W; i++) {
                if ((p[j] & (uint8_t)(0x80u >> i)) != 0u) {
                    GUI_SetPixel((uint8_t)(x + i), (uint8_t)(y + j), GUI_MODE_SET);
                }
            }
        }
    }
}

static void GUI_DrawCn(uint8_t x, uint8_t y, uint8_t idx)
{
    const uint8_t ROM *p = FontCnData(idx);
    uint8_t i;
    uint8_t j;

    for (j = 0u; j < FONT_CN_H; j++) {
        for (i = 0u; i < FONT_CN_W; i++) {
            if ((p[(uint8_t)(j * 2u + (uint8_t)(i >> 3))] & (uint8_t)(0x80u >> (i & 0x07u))) != 0u) {
                GUI_SetPixel((uint8_t)(x + i), (uint8_t)(y + j), GUI_MODE_SET);
            }
        }
    }
}

static uint8_t GUI_CharWidth(uint8_t c, GuiFont_t f)
{
    if (c >= 0x80u) {
        return FONT_CN_W;
    }
    return (f == GUI_FONT_16) ? (uint8_t)(ASCII_ADV * 2u) : (uint8_t)ASCII_ADV;
}

uint8_t GUI_TextWidth(const char ROM *s, GuiFont_t f)
{
    uint8_t w = 0u;
    uint8_t c;

    while ((c = (uint8_t)*s) != 0u) {
        w = (uint8_t)(w + GUI_CharWidth(c, f));
        s += (c >= 0x80u) ? 3u : 1u;
    }
    return w;
}

uint8_t GUI_TextWidthRam(const char *s, GuiFont_t f)
{
    uint8_t w = 0u;
    uint8_t c;

    while ((c = (uint8_t)*s) != 0u) {
        w = (uint8_t)(w + GUI_CharWidth(c, f));
        s += (c >= 0x80u) ? 3u : 1u;
    }
    return w;
}

uint8_t GUI_DrawText(uint8_t x, uint8_t y, const char ROM *s, GuiFont_t f)
{
    uint8_t c;
    uint8_t idx;

    while ((c = (uint8_t)*s) != 0u) {
        if (c < 0x80u) {
            GUI_DrawAscii(x, y, c, f);
            x = (uint8_t)(x + GUI_CharWidth(c, f));
            s++;
        } else {
            idx = FontCnFind((uint8_t)s[0u], (uint8_t)s[1u], (uint8_t)s[2u]);
            if (idx != FONT_CN_NOT_FOUND) {
                GUI_DrawCn(x, y, idx);
            }
            x = (uint8_t)(x + FONT_CN_W);
            s += 3u;
        }
    }
    return x;
}

/* 运行时(RAM)字符串版本：参数值、时间、版本号等 */
uint8_t GUI_DrawTextRam(uint8_t x, uint8_t y, const char *s, GuiFont_t f)
{
    uint8_t c;
    uint8_t idx;

    while ((c = (uint8_t)*s) != 0u) {
        if (c < 0x80u) {
            GUI_DrawAscii(x, y, c, f);
            x = (uint8_t)(x + GUI_CharWidth(c, f));
            s++;
        } else {
            idx = FontCnFind((uint8_t)s[0u], (uint8_t)s[1u], (uint8_t)s[2u]);
            if (idx != FONT_CN_NOT_FOUND) {
                GUI_DrawCn(x, y, idx);
            }
            x = (uint8_t)(x + FONT_CN_W);
            s += 3u;
        }
    }
    return x;
}

uint8_t GUI_DrawTextInv(uint8_t x, uint8_t y, const char ROM *s, GuiFont_t f)
{
    uint8_t x1 = GUI_DrawText(x, y, s, f);
    uint8_t h = (f == GUI_FONT_16) ? (uint8_t)FONT_CN_H : (uint8_t)ASCII_LINE_H;

    GUI_InvertRect(x, y, (uint8_t)(x1 - 1u), (uint8_t)(y + h - 1u));
    return x1;
}

uint8_t GUI_DrawTextCenter(uint8_t y, const char ROM *s, GuiFont_t f)
{
    uint8_t w = GUI_TextWidth(s, f);
    uint8_t x;

    x = (w >= LCD_WIDTH) ? 0u : (uint8_t)((LCD_WIDTH - w) / 2u);
    return GUI_DrawText(x, y, s, f);
}

uint8_t GUI_DrawTextCenterRam(uint8_t y, const char *s, GuiFont_t f)
{
    uint8_t w = GUI_TextWidthRam(s, f);
    uint8_t x;

    x = (w >= LCD_WIDTH) ? 0u : (uint8_t)((LCD_WIDTH - w) / 2u);
    return GUI_DrawTextRam(x, y, s, f);
}

uint8_t GUI_FontHeight(GuiFont_t f)
{
    return (f == GUI_FONT_16) ? (uint8_t)FONT_CN_H : (uint8_t)ASCII_LINE_H;
}

/*------------------------------------------------------------ 显存与刷新 */
void GUI_Clear(void)
{
    uint8_t p;
    uint8_t i;

    for (p = 0u; p < (uint8_t)LCD_PAGES; p++) {
        for (i = 0u; i < (uint8_t)LCD_WIDTH; i++) {
            GUI_VRam[p][i] = 0u;
        }
    }
    s_dirtyMask = 0xFFu;
}

void GUI_DirtyAll(void)
{
    s_dirtyMask = 0xFFu;
}

void GUI_Init(void)
{
    s_dirtyMask = 0u;
    s_flushTick = Timer_GetTick();
    GUI_Clear();
}

/* 每 GUI_FLUSH_PERIOD_MS 最多搬运 GUI_FLUSH_PAGES 页，单次耗时可控 */
void GUI_FlushTask(void)
{
    uint8_t p;
    uint8_t n = 0u;

    if (s_dirtyMask == 0u) {
        return;
    }
    if (!Timer_Beat(&s_flushTick, (uint16_t)GUI_FLUSH_PERIOD_MS)) {
        return;
    }
    for (p = 0u; p < (uint8_t)LCD_PAGES; p++) {
        if ((s_dirtyMask & (uint8_t)(1u << p)) != 0u) {
            LcdHal_WritePage(p, &GUI_VRam[p][0u]);
            s_dirtyMask &= (uint8_t)~(1u << p);
            n++;
            if (n >= (uint8_t)GUI_FLUSH_PAGES) {
                break;
            }
        }
    }
}
