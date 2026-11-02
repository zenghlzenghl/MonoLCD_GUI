#include "Bsp/LcdHal.h"
#include "Bsp/STC8H.h"
#include "Bsp/Timer.h"
#include "Config.h"

/*============================================================================
 * 128x64 单色屏页驱动（硬件 SPI）
 *   SSD1306：页地址模式，写命令 0xB0+page，列地址 0x00/0x10
 *   ST7565 ：页地址 0xB0+page，列地址 0x10/0x00
 * 换屏只需改 Config.h 的 LCD_CTRL
 *==========================================================================*/

#define SPI_SSIG    0x80u
#define SPI_EN      0x40u
#define SPI_MSTR    0x10u
#define SPI_CPHA    0x04u
#define SPI_CPOL    0x08u
#define SPI_SPR_MASK 0x03u

/* 等待 1 字节移位完成（约 1.3us@6MHz）。
 * 用计数上限兜底：万一屏幕没接/SPI 未工作也不会死等导致整机卡死 */
static void Spi_Write(uint8_t dat)
{
    uint16_t to = 0u;

    SPDAT = dat;
    while (((SPSTAT & 0x80u) == 0u) && (++to != 0u)) {
        ;
    }
    SPSTAT = 0xC0u;
}

static void Lcd_Cmd(uint8_t cmd)
{
    LCD_CS = 0u;
    LCD_DC = 0u;                    /* 命令 */
    Spi_Write(cmd);
    LCD_CS = 1u;
    LCD_DC = 1u;
}

static void Lcd_Data(const uint8_t XRAM *buf, uint16_t len)
{
    uint16_t i;

    LCD_CS = 0u;
    LCD_DC = 1u;                    /* 数据 */
    for (i = 0u; i < len; i++) {
        Spi_Write(buf[i]);
    }
    LCD_CS = 1u;
}

void LcdHal_Init(void)
{
    LCD_CS  = 1u;
    LCD_DC  = 1u;
    LCD_RES = 0u;

    /* 硬件 SPI：主机模式、忽略 SS、时钟空闲低、fosc/4 */
    P_SW1 = (uint8_t)((P_SW1 & (uint8_t)~0x0Cu) | (uint8_t)(LCD_SPI_SW & 0x0Cu));
    SPCTL = (uint8_t)(SPI_SSIG | SPI_EN | SPI_MSTR | SPI_SPR_MASK);
    SPSTAT = 0xC0u;

    Timer_DelayMs(2u);
    LCD_RES = 1u;
    Timer_DelayMs(5u);

#if (LCD_CTRL == LCD_CTRL_SSD1306)
    Lcd_Cmd(0xAEu);                 /* 关显示 */
    Lcd_Cmd(0x20u); Lcd_Cmd(0x02u); /* 页地址模式 */
    Lcd_Cmd(0x81u); Lcd_Cmd(0xCFu); /* 对比度 */
    Lcd_Cmd(0xA1u);                 /* 段重映射 */
    Lcd_Cmd(0xC8u);                 /* COM 扫描方向 */
    Lcd_Cmd(0xA6u);                 /* 正常显示 */
    Lcd_Cmd(0xA8u); Lcd_Cmd(0x3Fu); /* 复用比 64 */
    Lcd_Cmd(0xD3u); Lcd_Cmd(0x00u); /* 显示偏移 */
    Lcd_Cmd(0xD5u); Lcd_Cmd(0x80u); /* 时钟分频 */
    Lcd_Cmd(0xD9u); Lcd_Cmd(0xF1u); /* 预充电 */
    Lcd_Cmd(0xDAu); Lcd_Cmd(0x12u); /* COM 引脚配置 */
    Lcd_Cmd(0xDBu); Lcd_Cmd(0x40u); /* VCOMH */
    Lcd_Cmd(0x8Du); Lcd_Cmd(0x14u); /* 电荷泵 */
    Lcd_Cmd(0xAFu);                 /* 开显示 */
#elif (LCD_CTRL == LCD_CTRL_ST7565)
    Lcd_Cmd(0xE2u);                 /* 复位 */
    Lcd_Cmd(0xA0u);                 /* ADC 方向 */
    Lcd_Cmd(0xC8u);                 /* COM 方向 */
    Lcd_Cmd(0xA2u);                 /* LCD 偏压 */
    Lcd_Cmd(0x2Cu);                 /* 电源控制 */
    Lcd_Cmd(0x2Eu);
    Lcd_Cmd(0x2Fu);
    Lcd_Cmd(0x24u);                 /* 粗调对比度 */
    Lcd_Cmd(0x81u); Lcd_Cmd(0x20u); /* 细调对比度 */
    Lcd_Cmd(0xA6u);
    Lcd_Cmd(0x40u);                 /* 起始行 */
    Lcd_Cmd(0xAFu);
#else
    /* 无屏：只初始化 SPI，方便纯逻辑调试 */
#endif
}

void LcdHal_WritePage(uint8_t page, const uint8_t XRAM *buf)
{
    if (page >= (uint8_t)LCD_PAGES) {
        return;
    }

#if (LCD_CTRL == LCD_CTRL_SSD1306)
    Lcd_Cmd((uint8_t)(0xB0u + page));
    Lcd_Cmd(0x00u);                 /* 列低 4 位 */
    Lcd_Cmd(0x10u);                 /* 列高 4 位 */
#elif (LCD_CTRL == LCD_CTRL_ST7565)
    Lcd_Cmd((uint8_t)(0xB0u + page));
    Lcd_Cmd(0x10u);
    Lcd_Cmd(0x00u);
#else
    (void)buf;
    return;
#endif
    Lcd_Data(buf, (uint16_t)LCD_WIDTH);
}

void LcdHal_SetContrast(uint8_t val)
{
#if (LCD_CTRL == LCD_CTRL_SSD1306)
    Lcd_Cmd(0x81u);
    Lcd_Cmd(val);
#elif (LCD_CTRL == LCD_CTRL_ST7565)
    Lcd_Cmd(0x81u);
    Lcd_Cmd((uint8_t)(val >> 1u));
#else
    (void)val;
#endif
}

void LcdHal_SetInvert(uint8_t on)
{
#if (LCD_CTRL == LCD_CTRL_NONE)
    (void)on;
#else
    Lcd_Cmd((uint8_t)(on ? 0xA7u : 0xA6u));
#endif
}

void LcdHal_DisplayOn(uint8_t on)
{
#if (LCD_CTRL == LCD_CTRL_NONE)
    (void)on;
#else
    Lcd_Cmd((uint8_t)(on ? 0xAFu : 0xAEu));
#endif
}
