#ifndef __MENU_RES_H
#define __MENU_RES_H

#include "App/Menu.h"

/*============================================================================
 * 三层菜单资源表（全部存放于 Flash）
 *
 *   第 1 层 gPageRoot        主菜单
 *   第 2 层 s_pageXxx        分组
 *   第 3 层 叶子页           参数/动作/视图
 *==========================================================================*/

typedef enum {
    PARAM_HOUR = 0u,
    PARAM_MIN,
    PARAM_BRIGHT,
    PARAM_SLEEP,
    PARAM_INVERT,
    PARAM_LANG,
    PARAM_PERIOD,
    PARAM_CHANNEL,
    PARAM_NUM
} ParamId_t;

typedef enum {
    ACTION_RESET = 0u,
    ACTION_ERASE,
    ACTION_NUM
} ActionId_t;

typedef enum {
    VIEW_WAVE = 0u,
    VIEW_INFO,
    VIEW_VER,
    VIEW_BUILD,
    VIEW_SELFTEST,
    VIEW_UPTIME,
    VIEW_NUM
} ViewId_t;

extern const MenuPage_t ROM gPageRoot;

#endif /* __MENU_RES_H */
