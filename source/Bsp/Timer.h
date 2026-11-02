#ifndef __TIMER_H
#define __TIMER_H

#include "Type.h"

/*============================================================================
 * 系统时基：Timer0 产生 1ms 节拍，中断里只做 s_tick++
 * 所有"等待"都通过比较时基完成，主循环里不存在阻塞延时
 *==========================================================================*/

void     Timer_Init(void);
uint16_t Timer_GetTick(void);                       /* 中断安全的读法 */
uint8_t  Timer_Beat(uint16_t *last, uint16_t period);/* 周期到返回 1 并推进 */
uint16_t Timer_Elapsed(uint16_t since);
void     Timer_DelayMs(uint16_t ms);                /* 仅上电初始化可用 */

/*------------------------------------------------------------ 软件定时器 */
typedef struct {
    uint16_t start;
    uint16_t period;
    uint8_t  running;
} SoftTimer_t;

void    SoftTimer_Start(SoftTimer_t *t, uint16_t ms);
void    SoftTimer_Stop(SoftTimer_t *t);
uint8_t SoftTimer_Expired(SoftTimer_t *t);          /* 到点返回 1 并停止 */

#endif /* __TIMER_H */
