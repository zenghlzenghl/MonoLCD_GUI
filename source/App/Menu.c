#include "App/Menu.h"
#include "Bsp/Timer.h"

/*============================================================================
 * 菜单引擎实现
 *==========================================================================*/

#define MENU_MODE_LIST          0u
#define MENU_MODE_LEAF          1u
#define STATUS_LEN              8u
#define TOAST_LEN               16u

typedef struct {
    const MenuPage_t ROM *page;
    uint8_t index;
    uint8_t top;
} MenuFrame_t;

static MenuFrame_t  s_stack[MENU_DEPTH_MAX];
static uint8_t      s_depth;            /* 1..MENU_DEPTH_MAX */
static uint8_t      s_mode;             /* MENU_MODE_LIST / MENU_MODE_LEAF */
static uint8_t      s_leafType;
static uint8_t      s_leafId;
static const char ROM *s_leafText;
static int16_t     s_paramBackup;
static uint8_t     s_actSel;            /* 0=确认 1=取消 */
static uint8_t     s_dirty;
static uint16_t    s_renderTick;

static char        s_toast[TOAST_LEN];
static uint16_t    s_toastEnd;
static uint8_t     s_toastOn;

/*------------------------------------------------------------ 内部工具 */
static void Menu_Dirty(void)
{
    s_dirty = 1u;
}

void Menu_Refresh(void)
{
    s_dirty = 1u;
}

uint8_t Menu_Depth(void)
{
    return s_depth;
}

void Menu_Toast(const char ROM *text, uint16_t ms)
{
    uint8_t i = 0u;

    while ((i < (uint8_t)(TOAST_LEN - 1u)) && (text[i] != '\0')) {
        s_toast[i] = text[i];
        i++;
    }
    s_toast[i] = '\0';
    s_toastEnd = (uint16_t)(Timer_GetTick() + ms);
    s_toastOn  = 1u;
    Menu_Dirty();
}

static void Menu_ToastCheck(void)
{
    uint16_t now;

    if (!s_toastOn) {
        return;
    }
    now = Timer_GetTick();
    if ((uint16_t)(now - s_toastEnd) < 0x8000u) {   /* 已到结束时刻(考虑回绕) */
        s_toastOn = 0u;
        Menu_Dirty();                               /* 提示消失后自动重绘 */
    }
}

/*------------------------------------------------------------ 生命周期 */
void Menu_Init(const MenuPage_t ROM *root)
{
    uint8_t i;

    for (i = 0u; i < (uint8_t)MENU_DEPTH_MAX; i++) {
        s_stack[i].page  = NULL;
        s_stack[i].index = 0u;
        s_stack[i].top   = 0u;
    }
    s_stack[0].page = root;
    s_depth    = 1u;
    s_mode     = MENU_MODE_LIST;
    s_leafType = MENU_ITEM_SUBMENU;
    s_leafId   = 0u;
    s_leafText = NULL;
    s_actSel   = 0u;
    s_dirty    = 1u;
    s_toastOn  = 0u;
    s_renderTick = Timer_GetTick();
}

/*------------------------------------------------------------ 导航 */
static void Menu_EnterLeaf(const MenuItem_t ROM *item)
{
    s_leafType = item->type;
    s_leafId   = item->id;
    s_leafText = item->text;
    s_actSel   = 0u;
    if (item->type == MENU_ITEM_PARAM) {
        s_paramBackup = App_ParamGet(item->id);
    }
    if (item->type == MENU_ITEM_VIEW) {
        App_ViewEnter(item->id);
    }
    s_mode = MENU_MODE_LEAF;
}

static void Menu_Back(uint8_t cancel)
{
    if (s_mode == MENU_MODE_LEAF) {
        if ((s_leafType == MENU_ITEM_PARAM) && cancel) {
            App_ParamSet(s_leafId, s_paramBackup);       /* 放弃修改 */
        }
        s_mode = MENU_MODE_LIST;
        Menu_Dirty();
    } else if (s_depth > 1u) {
        s_depth--;
        Menu_Dirty();
    }
}

static void Menu_ListKey(uint8_t keyId)
{
    MenuFrame_t *f = &s_stack[s_depth - 1u];
    const MenuPage_t ROM *page = f->page;
    const MenuItem_t ROM *item;

    if (page == NULL) {
        return;
    }

    switch (keyId) {
    case KEY_UP:
        if (f->index > 0u) {
            f->index--;
            if (f->index < f->top) {
                f->top = f->index;
            }
            Menu_Dirty();
        }
        break;

    case KEY_DOWN:
        if ((uint8_t)(f->index + 1u) < page->count) {
            f->index++;
            if ((uint8_t)(f->index - f->top) >= (uint8_t)MENU_ROW_MAX) {
                f->top = (uint8_t)(f->index - (uint8_t)MENU_ROW_MAX + 1u);
            }
            Menu_Dirty();
        }
        break;

    case KEY_OK:
    case KEY_RIGHT:
        item = &page->items[f->index];
        if ((item->type == MENU_ITEM_SUBMENU) &&
            (s_depth < (uint8_t)MENU_DEPTH_MAX) &&
            (item->sub != NULL)) {
            s_depth++;
            s_stack[s_depth - 1u].page  = item->sub;
            s_stack[s_depth - 1u].index = 0u;
            s_stack[s_depth - 1u].top   = 0u;
            Menu_Dirty();
        } else if (item->type != MENU_ITEM_SUBMENU) {
            Menu_EnterLeaf(item);
            Menu_Dirty();
        }
        break;

    case KEY_LEFT:
        Menu_Back(FALSE);
        break;

    default:
        break;
    }
}

static void Menu_ParamAdjust(int16_t times)
{
    int16_t  min;
    int16_t  max;
    int16_t  v;
    uint16_t step;

    App_ParamInfo(s_leafId, &min, &max, &step);
    v = App_ParamGet(s_leafId);
    v = (int16_t)(v + (int16_t)((int16_t)times * (int16_t)step));
    if (v > max) {
        v = max;
    }
    if (v < min) {
        v = min;
    }
    App_ParamSet(s_leafId, v);
    Menu_Dirty();
}

static void Menu_LeafKey(uint8_t keyId)
{
    switch (s_leafType) {
    case MENU_ITEM_PARAM:
        switch (keyId) {
        case KEY_LEFT:  Menu_ParamAdjust(-1);  break;
        case KEY_RIGHT: Menu_ParamAdjust(1);   break;
        case KEY_DOWN:  Menu_ParamAdjust(-10); break;
        case KEY_UP:    Menu_ParamAdjust(10);  break;
        case KEY_OK:
            App_ParamCommit(s_leafId);
            Menu_Back(FALSE);
            break;
        default: break;
        }
        break;

    case MENU_ITEM_ACTION:
        switch (keyId) {
        case KEY_LEFT:
        case KEY_RIGHT:
            s_actSel = (uint8_t)(s_actSel ^ 1u);
            Menu_Dirty();
            break;
        case KEY_OK:
            if (s_actSel == 0u) {
                App_ActionConfirm(s_leafId);
            }
            Menu_Back(FALSE);
            break;
        default: break;
        }
        break;

    case MENU_ITEM_VIEW:
        if (keyId == KEY_OK) {
            Menu_Back(FALSE);
        } else {
            App_ViewKey(s_leafId, keyId, KEY_EVT_PRESS);
            Menu_Dirty();
        }
        break;

    default:
        break;
    }
}

void Menu_Key(uint8_t msg)
{
    uint8_t keyId = KEY_MSG_ID(msg);
    uint8_t evt   = KEY_MSG_EVT(msg);

    /* 长按 OK 统一作为"返回/取消" */
    if ((keyId == KEY_OK) && (evt == KEY_EVT_LONG)) {
        Menu_Back(TRUE);
        return;
    }
    if (evt == KEY_EVT_LONG) {
        return;
    }

    if (s_mode == MENU_MODE_LIST) {
        Menu_ListKey(keyId);
    } else {
        Menu_LeafKey(keyId);
    }
}

/*------------------------------------------------------------ 渲染 */
static void Menu_DrawTitleBar(void)
{
    char buf[STATUS_LEN];
    uint8_t w;
    const char ROM *title;

    if (s_mode == MENU_MODE_LIST) {
        title = (s_stack[s_depth - 1u].page != NULL) ? s_stack[s_depth - 1u].page->title : "";
    } else {
        title = (s_leafText != NULL) ? s_leafText : "";
    }
    GUI_DrawText(2u, 0u, title, GUI_FONT_16);

    App_StatusText(buf);
    w = GUI_TextWidthRam(buf, GUI_FONT_8);
    if (w < (uint8_t)(LCD_WIDTH - 40u)) {
        GUI_DrawTextRam((uint8_t)(LCD_WIDTH - w - 1u), 4u, buf, GUI_FONT_8);
    }
    GUI_HLine(0u, (uint8_t)(LCD_WIDTH - 1u), (uint8_t)(GUI_TITLE_H - 1u), GUI_MODE_SET);
}

static void Menu_DrawList(void)
{
    MenuFrame_t *f = &s_stack[s_depth - 1u];
    const MenuPage_t ROM *page = f->page;
    const MenuItem_t ROM *item;
    uint8_t i;
    uint8_t idx;
    uint8_t y;

    if (page == NULL) {
        return;
    }
    for (i = 0u; i < (uint8_t)MENU_ROW_MAX; i++) {
        idx = (uint8_t)(f->top + i);
        if (idx >= page->count) {
            break;
        }
        item = &page->items[idx];
        y = (uint8_t)(GUI_BODY_Y + (uint8_t)(i * (uint8_t)MENU_ROW_H));
        GUI_DrawText(2u, y, item->text, GUI_FONT_16);
        if (idx == f->index) {
            GUI_InvertRect(0u, y, (uint8_t)(LCD_WIDTH - 1u), (uint8_t)(y + (uint8_t)MENU_ROW_H - 1u));
        } else if (item->type == MENU_ITEM_SUBMENU) {
            GUI_DrawText((uint8_t)(LCD_WIDTH - 12u), y, ">", GUI_FONT_16);
        }
    }
    if (page->count > (uint8_t)MENU_ROW_MAX) {
        GUI_ScrollBar((uint8_t)(LCD_WIDTH - 4u), (uint8_t)GUI_BODY_Y,
                      (uint8_t)GUI_BODY_H, page->count, (uint8_t)MENU_ROW_MAX, f->top);
    }
}

static void Menu_DrawParam(void)
{
    char buf[TOAST_LEN];

    App_ParamFormat(s_leafId, buf);
    GUI_DrawTextCenterRam(22u, buf, GUI_FONT_16);
    GUI_DrawTextCenter(48u, "左右调整长按取消", GUI_FONT_8);
}

static void Menu_DrawAction(void)
{
    char buf[TOAST_LEN];

    App_ActionHint(s_leafId, buf);
    GUI_DrawTextCenterRam(18u, buf, GUI_FONT_16);

    /* 两个按钮：确认 / 取消（左右键切换，OK 执行） */
    GUI_Rect(26u, 44u, 61u, 61u, GUI_MODE_SET);
    GUI_Rect(66u, 44u, 101u, 61u, GUI_MODE_SET);
    GUI_DrawText(28u, 46u, "确认", GUI_FONT_16);
    GUI_DrawText(68u, 46u, "取消", GUI_FONT_16);
    if (s_actSel == 0u) {
        GUI_InvertRect(26u, 44u, 61u, 61u);
    } else {
        GUI_InvertRect(66u, 44u, 101u, 61u);
    }
}

static void Menu_DrawToast(void)
{
    uint8_t w = GUI_TextWidthRam(s_toast, GUI_FONT_16);
    uint8_t x;

    if (w == 0u) {
        return;
    }
    x = (w >= (uint8_t)(LCD_WIDTH - 8u)) ? 0u : (uint8_t)((LCD_WIDTH - w) / 2u);
    GUI_FillRect(0u, 26u, (uint8_t)(LCD_WIDTH - 1u), 45u, GUI_MODE_CLR);
    GUI_Rect(0u, 26u, (uint8_t)(LCD_WIDTH - 1u), 45u, GUI_MODE_SET);
    GUI_DrawTextRam(x, 28u, s_toast, GUI_FONT_16);
}

static void Menu_Render(void)
{
    GUI_Clear();
    Menu_DrawTitleBar();

    if (s_mode == MENU_MODE_LIST) {
        Menu_DrawList();
    } else {
        switch (s_leafType) {
        case MENU_ITEM_PARAM:  Menu_DrawParam();  break;
        case MENU_ITEM_ACTION: Menu_DrawAction(); break;
        case MENU_ITEM_VIEW:   App_ViewDraw(s_leafId); break;
        default: break;
        }
    }
    if (s_toastOn) {
        Menu_DrawToast();
    }
}

void Menu_Task(void)
{
    uint8_t anim;

    Menu_ToastCheck();

    anim = FALSE;
    if ((s_mode == MENU_MODE_LEAF) && (s_leafType == MENU_ITEM_VIEW)) {
        anim = App_ViewNeedDraw(s_leafId);
    }

    if (s_dirty) {
        Menu_Render();
        s_dirty = 0u;
        s_renderTick = Timer_GetTick();
    } else if (anim && Timer_Beat(&s_renderTick, (uint16_t)GUI_RENDER_PERIOD_MS)) {
        Menu_Render();                  /* 动画页按帧率重绘 */
    }
}
