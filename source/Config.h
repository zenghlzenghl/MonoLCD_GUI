#ifndef __CONFIG_H
#define __CONFIG_H

#include "Type.h"

/*============================================================================
 * 硬件与功能配置（移植时主要改这里）
 *==========================================================================*/

/* 系统时钟：必须与 STC-ISP 下载时选择的 IRC 频率一致 */
#ifndef FOSC
    #define FOSC                24000000UL
#endif

/*------------------------------------------------------------ LCD 屏 */
#define LCD_CTRL_NONE           0u
#define LCD_CTRL_SSD1306        1u      /* 常见 0.96/1.3 寸 OLED(SPI)   */
#define LCD_CTRL_ST7565         2u      /* 常见 12864 COG 液晶(SPI)     */

#ifndef LCD_CTRL
    #define LCD_CTRL            LCD_CTRL_SSD1306
#endif

#define LCD_WIDTH               128u
#define LCD_HEIGHT              64u
#define LCD_PAGES               (LCD_HEIGHT / 8u)     /* 8 页 */

/* 硬件 SPI：默认路由在 P1.2(SS)/P1.3(MOSI)/P1.4(MISO)/P1.5(SCLK)
 * 若实际 PCB 引脚不同，修改 Config.h 中的 LCD_SPI_SW 或改接排线 */
#define LCD_SPI_SW              0x00u   /* P_SW1 的 SPI_S[1:0]，0=默认组 */

/* 控制信号（推挽输出） */
SBIT_AT(0x92, LCD_CS);                  /* P1.2 片选，低有效 */
SBIT_AT(0xB2, LCD_DC);                  /* P3.2 数据/命令    */
SBIT_AT(0xB3, LCD_RES);                 /* P3.3 复位，低有效 */

/*------------------------------------------------------------ 按键 */
/* 5 个独立按键，低电平有效（准双向口，内部上拉） */
SBIT_AT(0x80, KEY_PIN_UP);              /* P0.0 */
SBIT_AT(0x81, KEY_PIN_DOWN);            /* P0.1 */
SBIT_AT(0x82, KEY_PIN_LEFT);            /* P0.2 */
SBIT_AT(0x83, KEY_PIN_RIGHT);           /* P0.3 */
SBIT_AT(0x84, KEY_PIN_OK);              /* P0.4 */

#define KEY_ACTIVE_LEVEL        0u      /* 按下时的电平 */
#define KEY_DEBOUNCE_MS         20u     /* 消抖       */
#define KEY_LONG_MS             600u    /* 长按判定   */
#define KEY_REPEAT_MS           130u    /* 连发间隔   */
#define KEY_FIFO_SIZE           8u      /* 事件队列深度 */

/*------------------------------------------------------------ 调度/刷新 */
#define GUI_FLUSH_PERIOD_MS     3u      /* 显存→屏幕 的周期       */
#define GUI_FLUSH_PAGES         2u      /* 每次最多刷几页(限制占用)*/
#define GUI_RENDER_PERIOD_MS    20u     /* 界面渲染周期(≈50FPS)   */
#define KEY_SCAN_PERIOD_MS      1u      /* 按键扫描周期           */
#define APP_TASK_PERIOD_MS      10u     /* 应用任务周期           */

/*------------------------------------------------------------ 功能开关 */
#define SPLASH_ENABLE           1u      /* 开机画面               */
#define SPLASH_TIME_MS          1200u
#define SLEEP_ENABLE            1u      /* 无操作熄屏             */
#define PARAM_SAVE_ENABLE       0u      /* 1=掉电保存(需实现 Nvm)  */
#define WDT_ENABLE              0u      /* 看门狗                 */
#define ADC_ENABLE              0u      /* 1=采样真实 ADC         */

/*------------------------------------------------------------ 菜单布局 */
#define GUI_TITLE_H             16u                             /* 标题栏高度 */
#define GUI_BODY_Y              GUI_TITLE_H
#define GUI_BODY_H              (LCD_HEIGHT - GUI_TITLE_H)      /* 48 */
#define MENU_ROW_H              16u                             /* 每行高度   */
#define MENU_ROW_MAX            (GUI_BODY_H / MENU_ROW_H)       /* 3 行可见   */
#define MENU_DEPTH_MAX          3u                              /* 三层菜单   */

#endif /* __CONFIG_H */
