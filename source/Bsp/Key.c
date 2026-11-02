#include "Bsp/Key.h"
#include "Bsp/STC8H.h"
#include "Bsp/Timer.h"
#include "Config.h"

/*============================================================================
 * 按键状态机
 *   每 1ms 扫一次：电平变化→重新计时消抖；稳定后判定 按下/长按/连发/松开
 *   事件写入 FIFO，队列满则丢弃（绝不等待）
 *==========================================================================*/

typedef struct {
    uint8_t  raw;           /* 上一次采样电平 */
    uint8_t  stable;        /* 消抖后的按下状态 */
    uint8_t  longSent;      /* 长按已发过 */
    uint16_t tmr;           /* 计时基准 */
} KeyCtx_t;

static KeyCtx_t s_ctx[KEY_NUM];
static uint8_t  s_fifo[KEY_FIFO_SIZE];
static uint8_t  s_head;
static uint8_t  s_tail;
static uint8_t  s_count;
static uint16_t s_lastPress;

static uint8_t Key_Level(uint8_t id)
{
    switch (id) {
    case KEY_UP:    return (uint8_t)KEY_PIN_UP;
    case KEY_DOWN:  return (uint8_t)KEY_PIN_DOWN;
    case KEY_LEFT:  return (uint8_t)KEY_PIN_LEFT;
    case KEY_RIGHT: return (uint8_t)KEY_PIN_RIGHT;
    case KEY_OK:    return (uint8_t)KEY_PIN_OK;
    default:        return (uint8_t)(!KEY_ACTIVE_LEVEL);
    }
}

static void Key_Push(uint8_t msg)
{
    if (s_count >= (uint8_t)KEY_FIFO_SIZE) {
        return;                             /* 满了就丢，不阻塞 */
    }
    s_fifo[s_tail] = msg;
    s_tail = (uint8_t)((s_tail + 1u) % (uint8_t)KEY_FIFO_SIZE);
    s_count++;
}

void Key_Init(void)
{
    uint8_t i;

    for (i = 0u; i < (uint8_t)KEY_NUM; i++) {
        s_ctx[i].raw      = (uint8_t)(!KEY_ACTIVE_LEVEL);
        s_ctx[i].stable   = 0u;
        s_ctx[i].longSent = 0u;
        s_ctx[i].tmr      = 0u;
    }
    s_head      = 0u;
    s_tail      = 0u;
    s_count     = 0u;
    s_lastPress = Timer_GetTick();
}

void Key_Scan(void)
{
    uint8_t  id;
    uint8_t  level;
    uint16_t now;
    KeyCtx_t *c;

    now = Timer_GetTick();

    for (id = 0u; id < (uint8_t)KEY_NUM; id++) {
        c = &s_ctx[id];
        level = Key_Level(id);

        if (level != c->raw) {              /* 电平变了，重新开始消抖计时 */
            c->raw = level;
            c->tmr = now;
            continue;
        }
        if (Timer_Elapsed(c->tmr) < (uint16_t)KEY_DEBOUNCE_MS) {
            continue;                       /* 还没稳定 */
        }

        if (level == (uint8_t)KEY_ACTIVE_LEVEL) {
            s_lastPress = now;
            if (!c->stable) {               /* 刚确认按下 */
                c->stable   = 1u;
                c->longSent = 0u;
                c->tmr      = now;
                Key_Push(KEY_MSG(id, KEY_EVT_PRESS));
            } else if (!c->longSent && (Timer_Elapsed(c->tmr) >= (uint16_t)KEY_LONG_MS)) {
                c->longSent = 1u;           /* 长按 */
                c->tmr      = now;
                Key_Push(KEY_MSG(id, KEY_EVT_LONG));
            } else if (c->longSent && (Timer_Elapsed(c->tmr) >= (uint16_t)KEY_REPEAT_MS)) {
                c->tmr = now;               /* 连发 */
                Key_Push(KEY_MSG(id, KEY_EVT_REPEAT));
            }
        } else if (c->stable) {
            c->stable = 0u;
            c->tmr    = now;
            Key_Push(KEY_MSG(id, KEY_EVT_RELEASE));
        }
    }
}

uint8_t Key_Get(uint8_t *msg)
{
    uint8_t m;

    if (s_count == 0u) {
        return 0u;
    }
    m = s_fifo[s_head];
    s_head = (uint8_t)((s_head + 1u) % (uint8_t)KEY_FIFO_SIZE);
    s_count--;
    *msg = m;
    return 1u;
}

uint16_t Key_IdleMs(void)
{
    return Timer_Elapsed(s_lastPress);
}
