#ifndef __APP_H
#define __APP_H

#include "Type.h"

/*============================================================================
 * 应用层：参数、视图页、动作、软时钟、熄屏、开机画面
 * 全部以"状态 + 时基"方式实现，不含任何阻塞等待
 *==========================================================================*/

void    App_Init(void);
void    App_Task(void);                 /* 主循环周期调用(10ms) */

uint8_t App_SplashBusy(void);
void    App_SplashTask(void);
void    App_SplashSkip(void);

uint8_t App_Sleeping(void);
void    App_Wake(void);

#endif /* __APP_H */
