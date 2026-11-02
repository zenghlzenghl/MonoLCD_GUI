#include "Bsp/Timer.h"
#include "Bsp/STC8H.h"
#include "Config.h"

/*============================================================================
 * 1ms 时基（Timer0，1T 模式，16 位自动重装载）
 *==========================================================================*/

static volatile uint16_t IRAM s_tick;

#define T1MS_RELOAD     ((uint16_t)(65536UL - (FOSC / 1000UL)))

void Timer0_Isr(void) INTERRUPT_AT(1)
{
    TF0 = 0u;                       /* STC8H 需软件清除溢出标志 */
    s_tick++;
}

void Timer_Init(void)
{
    AUXR |= 0x80u;                  /* T0x12=1：1T 模式 */
    AUXR &= (uint8_t)~0x02u;        /* EXTRAM=0：使用内部扩展 RAM */

    TMOD &= 0xF0u;                  /* T0：16 位自动重装载 */
    TMOD |= 0x00u;
    TH0 = (uint8_t)(T1MS_RELOAD >> 8);
    TL0 = (uint8_t)(T1MS_RELOAD & 0xFFu);
    TF0 = 0u;
    TR0 = 1u;
    ET0 = 1u;
    s_tick = 0u;
    EA = 1u;                        /* 上电初始化要用 Timer_DelayMs，此处先开总中断 */
}

uint16_t Timer_GetTick(void)
{
    uint16_t t;

    EA = 0u;
    t = s_tick;
    EA = 1u;
    return t;
}

/* 到周期返回 1（并推进 last），未到返回 0 —— 非阻塞节拍的核心 */
uint8_t Timer_Beat(uint16_t *last, uint16_t period)
{
    uint16_t now = Timer_GetTick();

    if ((uint16_t)(now - *last) >= period) {
        *last = now;
        return 1u;
    }
    return 0u;
}

uint16_t Timer_Elapsed(uint16_t since)
{
    return (uint16_t)(Timer_GetTick() - since);
}

/* 仅供上电初始化（中断已开、界面还没跑起来）使用 */
void Timer_DelayMs(uint16_t ms)
{
    uint16_t start = Timer_GetTick();

    while (Timer_Elapsed(start) < ms) {
        ;
    }
}

/*------------------------------------------------------------ 软件定时器 */
void SoftTimer_Start(SoftTimer_t *t, uint16_t ms)
{
    t->start   = Timer_GetTick();
    t->period  = ms;
    t->running = 1u;
}

void SoftTimer_Stop(SoftTimer_t *t)
{
    t->running = 0u;
}

uint8_t SoftTimer_Expired(SoftTimer_t *t)
{
    if (!t->running) {
        return 0u;
    }
    if (Timer_Elapsed(t->start) >= t->period) {
        t->running = 0u;
        return 1u;
    }
    return 0u;
}
