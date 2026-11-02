#ifndef __MENU_H
#define __MENU_H

#include "Type.h"
#include "Config.h"
#include "GUI/GUI.h"
#include "Bsp/Key.h"

/*============================================================================
 * 三层菜单引擎（事件驱动 + 无阻塞渲染）
 *
 *   第 1 层：主菜单列表
 *   第 2 层：分组列表
 *   第 3 层：叶子页（参数编辑 / 动作确认 / 自定义视图）
 *
 * 所有按键处理都是"取事件→改状态→置脏标记"，渲染在主循环中异步完成，
 * 因此菜单里不存在任何等待、延时或 while 死循环。
 *==========================================================================*/

typedef enum {
    MENU_ITEM_SUBMENU = 0u,     /* 进入下一层列表 */
    MENU_ITEM_PARAM,            /* 参数编辑页     */
    MENU_ITEM_ACTION,           /* 动作确认页     */
    MENU_ITEM_VIEW              /* 自定义视图页   */
} MenuItemType_t;

typedef struct MenuPage MenuPage_t;

typedef struct {
    const char ROM *text;               /* 标题(UTF-8)，可中英混排 */
    uint8_t type;                       /* MenuItemType_t */
    uint8_t id;                         /* 参数/视图/动作 ID */
    const MenuPage_t ROM *sub;          /* 仅 SUBMENU 有效 */
} MenuItem_t;

struct MenuPage {
    const char ROM *title;
    const MenuItem_t ROM *items;
    uint8_t count;
};

/*------------------------------------------------------------ 引擎接口 */
void    Menu_Init(const MenuPage_t ROM *root);
void    Menu_Key(uint8_t msg);          /* 投递一条按键消息 */
void    Menu_Task(void);                /* 主循环周期调用：按需重绘 */
void    Menu_Refresh(void);             /* 请求重绘 */
void    Menu_Toast(const char ROM *text, uint16_t ms);   /* 非阻塞提示条 */
uint8_t Menu_Depth(void);

/*------------------------------------------------------------ 应用层回调(由 App.c 实现) */
/* 参数页 */
void    App_ParamInfo(uint8_t id, int16_t *min, int16_t *max, uint16_t *step);
int16_t App_ParamGet(uint8_t id);
void    App_ParamSet(uint8_t id, int16_t v);
void    App_ParamFormat(uint8_t id, char *buf);
void    App_ParamCommit(uint8_t id);            /* 确认：持久化 */
/* 动作页 */
void    App_ActionHint(uint8_t id, char *buf);
void    App_ActionConfirm(uint8_t id);          /* 二次确认后执行 */
/* 视图页 */
void    App_ViewEnter(uint8_t id);              /* 进入视图页时调用一次 */
void    App_ViewDraw(uint8_t id);               /* 只画正文区(y=16~63) */
void    App_ViewKey(uint8_t id, uint8_t keyId, uint8_t evt);
uint8_t App_ViewNeedDraw(uint8_t id);           /* 1=动画页，需要持续重绘 */
/* 标题栏右侧状态 */
void    App_StatusText(char *buf);

#endif /* __MENU_H */
