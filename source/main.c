#include "Type.h"
#include "Config.h"
#include "Bsp/STC8H.h"
#include "Bsp/Timer.h"
#include "Bsp/Key.h"
#include "Bsp/LcdHal.h"
#include "GUI/GUI.h"
#include "App/Menu.h"
#include "App/MenuRes.h"
#include "App/App.h"

/*============================================================================
 * 主调度器
 *
 * 设计原则：主循环里没有任何阻塞延时，每个任务都是"看一眼时基→干一点活→立刻返回"
 *   Key_Scan   1ms   按键状态机，结果放入事件队列
 *   App_Task   10ms  软时钟/采样/自检/熄屏
 *   Menu_Task  按需   只有在状态变化或动画页时才重绘
 *   GUI_Flush  3ms   把"脏页"分批搬到屏幕
 *==========================================================================*/

static uint16_t s_keyTick;
static uint16_t s_appTick;

static void Board_Init(void)
{
    /* LCD 控制口：推挽输出(P1.2 CS / P1.3 MOSI / P1.5 SCLK / P3.2 DC / P3.3 RES) */
    P1M0 |= 0x2Cu;
    P1M1 &= (uint8_t)~0x2Cu;
    P3M0 |= 0x0Cu;
    P3M1 &= (uint8_t)~0x0Cu;

    /* 按键口：准双向(内部上拉) */
    P0M0 &= (uint8_t)~0x1Fu;
    P0M1 &= (uint8_t)~0x1Fu;
}

void main(void)
{
    uint8_t msg;

    Board_Init();
    Timer_Init();
    Key_Init();
    LcdHal_Init();
    GUI_Init();
    App_Init();
    Menu_Init(&gPageRoot);

    EA = 1u;                                /* 开总中断，1ms 时基开始工作 */

    s_keyTick = Timer_GetTick();
    s_appTick = s_keyTick;

    while (1u) {
        /* 1ms：按键状态机 */
        if (Timer_Beat(&s_keyTick, (uint16_t)KEY_SCAN_PERIOD_MS)) {
            Key_Scan();
        }

        /* 按键事件：先交给系统功能(唤醒/跳过开机画面)，再交给菜单 */
        while (Key_Get(&msg)) {
            if (App_Sleeping()) {
                App_Wake();
                continue;
            }
            if (App_SplashBusy()) {
                App_SplashSkip();
                continue;
            }
            Menu_Key(msg);
        }

        /* 10ms：应用任务 */
        if (Timer_Beat(&s_appTick, (uint16_t)APP_TASK_PERIOD_MS)) {
            App_Task();
        }

        /* 界面：开机画面优先，其次菜单 */
        if (App_SplashBusy()) {
            App_SplashTask();
            GUI_FlushTask();
        } else if (!App_Sleeping()) {
            Menu_Task();
            GUI_FlushTask();            /* 显存 → 屏幕，熄屏时不再搬运 */
        }

#if WDT_ENABLE
        WDT_CONTR = 0x3Cu;                  /* 喂狗 */
#endif
    }
}
