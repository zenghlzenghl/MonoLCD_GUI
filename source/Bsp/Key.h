#ifndef __KEY_H
#define __KEY_H

#include "Type.h"

/*============================================================================
 * 按键：1ms 扫描状态机（消抖/长按/连发），结果放入事件 FIFO
 * 取事件是非阻塞的：没事件 Key_Get 直接返回 0
 *==========================================================================*/

typedef enum {
    KEY_UP = 0u,
    KEY_DOWN,
    KEY_LEFT,
    KEY_RIGHT,
    KEY_OK,
    KEY_NUM
} KeyId_t;

typedef enum {
    KEY_EVT_PRESS = 0u,     /* 按下         */
    KEY_EVT_LONG,           /* 长按到点     */
    KEY_EVT_REPEAT,         /* 长按连发     */
    KEY_EVT_RELEASE         /* 松开         */
} KeyEvt_t;

/* 事件打包成一个字节：低 4 位按键 ID，高 4 位事件类型 */
#define KEY_MSG(id, evt)    ((uint8_t)(((uint8_t)(id) & 0x0Fu) | (((uint8_t)(evt) & 0x0Fu) << 4u)))
#define KEY_MSG_ID(msg)     ((uint8_t)((msg) & 0x0Fu))
#define KEY_MSG_EVT(msg)    ((uint8_t)(((msg) >> 4u) & 0x0Fu))

void     Key_Init(void);
void     Key_Scan(void);                    /* 每 1ms 调用一次 */
uint8_t  Key_Get(uint8_t *msg);             /* 取一条事件，无事件返回 0 */
uint16_t Key_IdleMs(void);                  /* 距上次操作的毫秒数（熄屏用） */

#endif /* __KEY_H */
