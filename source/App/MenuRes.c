#include "App/MenuRes.h"

/*------------------------------------------------------------ 第 2 层 */
static const MenuItem_t ROM s_itemsSys[] = {
    {"小时设置", MENU_ITEM_PARAM,  PARAM_HOUR,    NULL},
    {"分钟设置", MENU_ITEM_PARAM,  PARAM_MIN,     NULL},
    {"恢复出厂", MENU_ITEM_ACTION, ACTION_RESET,  NULL},
    {"设备信息", MENU_ITEM_VIEW,   VIEW_INFO,     NULL},
};
static const MenuPage_t ROM s_pageSys = {"系统设置", s_itemsSys, NELEM(s_itemsSys)};

static const MenuItem_t ROM s_itemsDisp[] = {
    {"亮度调节", MENU_ITEM_PARAM,  PARAM_BRIGHT,  NULL},
    {"熄屏时间", MENU_ITEM_PARAM,  PARAM_SLEEP,   NULL},
    {"反色显示", MENU_ITEM_PARAM,  PARAM_INVERT,  NULL},
    {"界面语言", MENU_ITEM_PARAM,  PARAM_LANG,    NULL},
};
static const MenuPage_t ROM s_pageDisp = {"显示设置", s_itemsDisp, NELEM(s_itemsDisp)};

static const MenuItem_t ROM s_itemsSample[] = {
    {"采样周期", MENU_ITEM_PARAM,  PARAM_PERIOD,  NULL},
    {"通道选择", MENU_ITEM_PARAM,  PARAM_CHANNEL, NULL},
    {"启动采样", MENU_ITEM_VIEW,   VIEW_WAVE,     NULL},
    {"存储管理", MENU_ITEM_ACTION, ACTION_ERASE,  NULL},
};
static const MenuPage_t ROM s_pageSample = {"数据采集", s_itemsSample, NELEM(s_itemsSample)};

static const MenuItem_t ROM s_itemsAbout[] = {
    {"固件版本", MENU_ITEM_VIEW,   VIEW_VER,      NULL},
    {"编译时间", MENU_ITEM_VIEW,   VIEW_BUILD,    NULL},
    {"硬件自检", MENU_ITEM_VIEW,   VIEW_SELFTEST, NULL},
    {"运行时间", MENU_ITEM_VIEW,   VIEW_UPTIME,   NULL},
};
static const MenuPage_t ROM s_pageAbout = {"关于设备", s_itemsAbout, NELEM(s_itemsAbout)};

/*------------------------------------------------------------ 第 1 层 */
static const MenuItem_t ROM s_itemsRoot[] = {
    {"系统设置", MENU_ITEM_SUBMENU, 0u, &s_pageSys},
    {"显示设置", MENU_ITEM_SUBMENU, 0u, &s_pageDisp},
    {"数据采集", MENU_ITEM_SUBMENU, 0u, &s_pageSample},
    {"关于设备", MENU_ITEM_SUBMENU, 0u, &s_pageAbout},
};

const MenuPage_t ROM gPageRoot = {"主菜单", s_itemsRoot, NELEM(s_itemsRoot)};
