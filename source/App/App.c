#include "App/App.h"
#include "App/Menu.h"
#include "App/MenuRes.h"
#include "GUI/GUI.h"
#include "Bsp/Key.h"
#include "Bsp/LcdHal.h"
#include "Bsp/Timer.h"
#include "Config.h"

/*============================================================================
 * 应用层实现
 *==========================================================================*/

#define WAVE_POINTS         64u
#define WAVE_BOTTOM         47u
#define TEST_STEPS          4u
#define SAVE_DELAY_MS       3000u

typedef struct {
    int16_t  min;
    int16_t  max;
    uint16_t step;
} ParamInfo_t;

/*------------------------------------------------------------ 参数存储 */
static int16_t XRAM s_param[PARAM_NUM];

static const int16_t ROM s_def[PARAM_NUM] = {
    12,         /* 小时 */
    30,         /* 分钟 */
    80,         /* 亮度 */
    30,         /* 熄屏时间(s) */
    0,          /* 反色 */
    0,          /* 语言: 0=中文 1=EN */
    100,        /* 采样周期(ms) */
    1           /* 通道 */
};

static const ParamInfo_t ROM s_info[PARAM_NUM] = {
    {0,   23,   1},
    {0,   59,   1},
    {10,  100,  5},
    {10,  180,  10},
    {0,   1,    1},
    {0,   1,    1},
    {10,  1000, 10},
    {1,   4,    1}
};

/*------------------------------------------------------------ 运行时状态 */
static uint8_t  s_rtcH;
static uint8_t  s_rtcM;
static uint8_t  s_rtcS;
static uint16_t s_rtcAcc;               /* ms 累计 */
static uint16_t s_upSec;                /* 运行时间 */

static uint8_t XRAM s_wave[WAVE_POINTS];
static uint8_t s_waveHead;
static uint8_t s_waveRun;
static uint8_t s_phase;
static uint16_t s_waveTick;

static uint8_t     s_testStep;
static uint8_t     s_testRun;
static SoftTimer_t s_testTmr;

static uint8_t  s_splash;
static uint16_t s_splashEnd;
static uint16_t s_splashTick;

static uint8_t     s_sleep;
static uint8_t     s_saveDirty;
static SoftTimer_t s_saveTmr;

/* 32 点正弦表，幅值 1~31，用于波形页演示(可替换为真实 ADC 采样) */
static const uint8_t ROM s_sine[32] = {
    16, 19, 22, 24, 27, 28, 30, 31, 31, 31, 30, 28, 27, 24, 22, 19,
    16, 13, 10,  8,  5,  4,  2,  1,  1,  1,  2,  4,  5,  8, 10, 13
};

static const char ROM s_testName[TEST_STEPS][13] = {
    "检查显示", "检查按键", "检查存储", "检查通信"
};

/*------------------------------------------------------------ 小工具 */
static void StrCpyRam(char *dst, const char *src)
{
    while (*src != '\0') {
        *dst++ = *src++;
    }
    *dst = '\0';
}

static void StrCpyRom(char *dst, const char ROM *src)
{
    while (*src != '\0') {
        *dst++ = *src++;
    }
    *dst = '\0';
}

static void StrCatRam(char *dst, const char *src)
{
    while (*dst != '\0') {
        dst++;
    }
    StrCpyRam(dst, src);
}

static void StrCatRom(char *dst, const char ROM *src)
{
    while (*dst != '\0') {
        dst++;
    }
    StrCpyRom(dst, src);
}

/* 无前导零的十进制转换 */
static void U16ToStr(uint16_t v, char *out)
{
    char tmp[6];
    uint8_t i = 0u;
    uint8_t j;

    do {
        tmp[i++] = (char)('0' + (v % 10u));
        v /= 10u;
    } while ((v != 0u) && (i < 5u));

    for (j = 0u; j < i; j++) {
        out[j] = tmp[(uint8_t)(i - 1u - j)];
    }
    out[i] = '\0';
}

static void TimeToStr(uint8_t h, uint8_t m, char *out)
{
    out[0] = (char)('0' + (h / 10u));
    out[1] = (char)('0' + (h % 10u));
    out[2] = ':';
    out[3] = (char)('0' + (m / 10u));
    out[4] = (char)('0' + (m % 10u));
    out[5] = '\0';
}

static void UptimeToStr(uint16_t sec, char *out)
{
    uint16_t h = (uint16_t)(sec / 3600u);
    uint16_t m = (uint16_t)((sec % 3600u) / 60u);
    uint16_t s = (uint16_t)(sec % 60u);

    out[0] = (char)('0' + (h / 10u));
    out[1] = (char)('0' + (h % 10u));
    out[2] = ':';
    out[3] = (char)('0' + (m / 10u));
    out[4] = (char)('0' + (m % 10u));
    out[5] = ':';
    out[6] = (char)('0' + (s / 10u));
    out[7] = (char)('0' + (s % 10u));
    out[8] = '\0';
}

/*------------------------------------------------------------ 参数回调 */
void App_ParamInfo(uint8_t id, int16_t *min, int16_t *max, uint16_t *step)
{
    if (id >= (uint8_t)PARAM_NUM) {
        return;
    }
    *min  = s_info[id].min;
    *max  = s_info[id].max;
    *step = s_info[id].step;
}

int16_t App_ParamGet(uint8_t id)
{
    if (id >= (uint8_t)PARAM_NUM) {
        return 0;
    }
    return s_param[id];
}

void App_ParamSet(uint8_t id, int16_t v)
{
    if (id >= (uint8_t)PARAM_NUM) {
        return;
    }
    s_param[id] = v;

    switch (id) {
    case PARAM_BRIGHT:                                  /* 亮度 → 对比度寄存器 */
        LcdHal_SetContrast((uint8_t)(((uint16_t)v * 255u) / 100u));
        break;
    case PARAM_INVERT:                                  /* 硬件反色，零开销 */
        LcdHal_SetInvert((uint8_t)(v ? 1u : 0u));
        break;
    case PARAM_HOUR:
        s_rtcH = (uint8_t)v;
        break;
    case PARAM_MIN:
        s_rtcM = (uint8_t)v;
        break;
    default:
        break;
    }
    s_saveDirty = 1u;
    SoftTimer_Start(&s_saveTmr, SAVE_DELAY_MS);
}

void App_ParamFormat(uint8_t id, char *buf)
{
    char tmp[8];
    int16_t v = App_ParamGet(id);

    buf[0] = '\0';
    switch (id) {
    case PARAM_HOUR:
    case PARAM_MIN:
        U16ToStr((uint16_t)v, buf);
        break;
    case PARAM_BRIGHT:
        U16ToStr((uint16_t)v, buf);
        StrCatRom(buf, "%");
        break;
    case PARAM_SLEEP:
        U16ToStr((uint16_t)v, buf);
        StrCatRom(buf, "S");
        break;
    case PARAM_PERIOD:
        U16ToStr((uint16_t)v, buf);
        StrCatRom(buf, "MS");
        break;
    case PARAM_CHANNEL:
        StrCpyRom(buf, "CH");
        U16ToStr((uint16_t)v, tmp);
        StrCatRam(buf, tmp);
        break;
    case PARAM_INVERT:
        StrCpyRom(buf, v ? "开" : "关");
        break;
    case PARAM_LANG:
        StrCpyRom(buf, v ? "EN" : "中文");
        break;
    default:
        break;
    }
}

void App_ParamCommit(uint8_t id)
{
    (void)id;
    Menu_Toast("已保存", 1000u);          /* 提示也是非阻塞的 */
}

/*------------------------------------------------------------ 动作回调 */
void App_ActionHint(uint8_t id, char *buf)
{
    switch (id) {
    case ACTION_RESET: StrCpyRom(buf, "恢复出厂设置"); break;
    case ACTION_ERASE: StrCpyRom(buf, "擦除采样记录"); break;
    default:           StrCpyRom(buf, "请确认操作");   break;
    }
}

void App_ActionConfirm(uint8_t id)
{
    uint8_t i;

    switch (id) {
    case ACTION_RESET:
        for (i = 0u; i < (uint8_t)PARAM_NUM; i++) {
            App_ParamSet(i, s_def[i]);
        }
        s_upSec = 0u;
        Menu_Toast("已恢复出厂", 1500u);
        break;

    case ACTION_ERASE:
        for (i = 0u; i < (uint8_t)WAVE_POINTS; i++) {
            s_wave[i] = 16u;
        }
        s_waveHead = 0u;
        Menu_Toast("已擦除", 1500u);
        break;

    default:
        break;
    }
}

/*------------------------------------------------------------ 视图页 */
void App_ViewEnter(uint8_t id)
{
    switch (id) {
    case VIEW_WAVE:
        s_waveRun = 1u;
        s_waveTick = Timer_GetTick();
        break;
    case VIEW_SELFTEST:                  /* 多步自检：分步执行，每步 400ms，全程不阻塞 */
        s_testStep = 0u;
        s_testRun  = 1u;
        SoftTimer_Start(&s_testTmr, 400u);
        break;
    default:
        break;
    }
}

uint8_t App_ViewNeedDraw(uint8_t id)
{
    return ((id == VIEW_WAVE) || (id == VIEW_SELFTEST) || (id == VIEW_UPTIME)) ? 1u : 0u;
}

static void DrawViewInfo(void)
{
    GUI_DrawTextCenter(18u, "STC8H8K64U", GUI_FONT_16);
    GUI_DrawTextCenter(38u, "128x64 MONO LCD", GUI_FONT_8);
    GUI_DrawTextCenter(50u, "NON-BLOCK GUI DEMO", GUI_FONT_8);
}

static void DrawViewVer(void)
{
    GUI_DrawTextCenter(24u, "V1.00", GUI_FONT_16);
    GUI_DrawTextCenter(48u, "STC8H  12864  GUI", GUI_FONT_8);
}

static void DrawViewBuild(void)
{
    GUI_DrawTextCenter(24u, "2026-11-02", GUI_FONT_16);
    GUI_DrawTextCenter(48u, "BUILD 14:30:00", GUI_FONT_8);
}

static void DrawViewUpTime(void)
{
    char buf[10];

    UptimeToStr(s_upSec, buf);
    GUI_DrawTextCenterRam(24u, buf, GUI_FONT_16);
    GUI_DrawTextCenter(48u, "UPTIME", GUI_FONT_8);
}

static void DrawViewWave(void)
{
    char buf[20];
    char tmp[6];
    uint8_t i;
    uint8_t idx;
    uint8_t v0;
    uint8_t v1;

    GUI_HLine(0u, (uint8_t)(LCD_WIDTH - 1u), WAVE_BOTTOM, GUI_MODE_SET);

    for (i = 0u; i < (uint8_t)(WAVE_POINTS - 1u); i++) {
        idx = (uint8_t)((uint8_t)(s_waveHead + i) % WAVE_POINTS);
        v0  = s_wave[idx];
        idx = (uint8_t)((uint8_t)(s_waveHead + i + 1u) % WAVE_POINTS);
        v1  = s_wave[idx];
        GUI_Line((int16_t)(i * 2), (int16_t)(WAVE_BOTTOM - v0),
                 (int16_t)((i + 1u) * 2), (int16_t)(WAVE_BOTTOM - v1), GUI_MODE_SET);
    }

    StrCpyRom(buf, "CH");
    U16ToStr((uint16_t)s_param[PARAM_CHANNEL], tmp);
    StrCatRam(buf, tmp);
    StrCatRom(buf, " ");
    U16ToStr((uint16_t)s_param[PARAM_PERIOD], tmp);
    StrCatRam(buf, tmp);
    StrCatRom(buf, "MS ");
    StrCatRom(buf, s_waveRun ? "RUN" : "HOLD");
    GUI_DrawTextRam(0u, 52u, buf, GUI_FONT_8);
}

static void DrawViewSelfTest(void)
{
    uint8_t percent;
    uint8_t step = s_testStep;

    if (step > TEST_STEPS) {
        step = TEST_STEPS;
    }
    GUI_DrawTextCenter(18u, s_testName[(step < TEST_STEPS) ? step : (TEST_STEPS - 1u)], GUI_FONT_16);

    if (s_testRun) {
        GUI_DrawTextCenter(36u, "自检进行中", GUI_FONT_8);
    } else {
        GUI_DrawTextCenter(36u, "自检完成", GUI_FONT_8);
    }
    percent = (uint8_t)((uint16_t)step * 100u / TEST_STEPS);
    GUI_ProgressBar(14u, 52u, 100u, 8u, percent);
}

void App_ViewDraw(uint8_t id)
{
    switch (id) {
    case VIEW_WAVE:     DrawViewWave();     break;
    case VIEW_INFO:     DrawViewInfo();     break;
    case VIEW_VER:      DrawViewVer();      break;
    case VIEW_BUILD:    DrawViewBuild();    break;
    case VIEW_SELFTEST: DrawViewSelfTest(); break;
    case VIEW_UPTIME:   DrawViewUpTime();   break;
    default: break;
    }
}

void App_ViewKey(uint8_t id, uint8_t keyId, uint8_t evt)
{
    (void)evt;
    if (id == VIEW_WAVE) {
        if ((keyId == KEY_LEFT) || (keyId == KEY_RIGHT)) {
            s_waveRun = (uint8_t)(s_waveRun ^ 1u);       /* 启停采样 */
        } else if (keyId == KEY_UP) {
            s_waveRun = 1u;
        } else if (keyId == KEY_DOWN) {
            s_waveRun = 0u;
        }
    }
}

/*------------------------------------------------------------ 标题栏状态 */
void App_StatusText(char *buf)
{
    TimeToStr(s_rtcH, s_rtcM, buf);
}

/*------------------------------------------------------------ 开机画面 */
uint8_t App_SplashBusy(void)
{
    return s_splash;
}

void App_SplashSkip(void)
{
    s_splash = 0u;
    Menu_Refresh();
}

static void DrawSplash(uint8_t percent)
{
    GUI_Clear();
    GUI_DrawTextCenter(8u, "12864 GUI", GUI_FONT_16);
    GUI_Rect(0u, 0u, (uint8_t)(LCD_WIDTH - 1u), (uint8_t)(LCD_HEIGHT - 1u), GUI_MODE_SET);
    GUI_DrawTextCenter(30u, "STC8H  MONO  LCD", GUI_FONT_8);
    GUI_ProgressBar(14u, 44u, 100u, 10u, percent);
}

void App_SplashTask(void)
{
    uint16_t elapsed;

    if (!s_splash) {
        return;
    }
    elapsed = Timer_Elapsed((uint16_t)(s_splashEnd - (uint16_t)SPLASH_TIME_MS));
    if (elapsed >= (uint16_t)SPLASH_TIME_MS) {
        s_splash = 0u;
        Menu_Refresh();
        return;
    }
    if (Timer_Beat(&s_splashTick, 50u)) {
        DrawSplash((uint8_t)((uint16_t)elapsed * 100u / (uint16_t)SPLASH_TIME_MS));
    }
}

/*------------------------------------------------------------ 熄屏 */
uint8_t App_Sleeping(void)
{
    return s_sleep;
}

void App_Wake(void)
{
    if (s_sleep) {
        s_sleep = 0u;
        LcdHal_DisplayOn(1u);
        GUI_DirtyAll();
        Menu_Refresh();
    }
}

/*------------------------------------------------------------ 初始化与任务 */
void App_Init(void)
{
    uint8_t i;

    for (i = 0u; i < (uint8_t)PARAM_NUM; i++) {
        s_param[i] = s_def[i];
    }
    for (i = 0u; i < (uint8_t)WAVE_POINTS; i++) {
        s_wave[i] = 16u;
    }
    s_waveHead = 0u;
    s_waveRun  = 0u;
    s_phase    = 0u;
    s_rtcH = (uint8_t)s_param[PARAM_HOUR];
    s_rtcM = (uint8_t)s_param[PARAM_MIN];
    s_rtcS = 0u;
    s_rtcAcc = 0u;
    s_upSec = 0u;
    s_testStep = 0u;
    s_testRun  = 0u;
    s_sleep = 0u;
    s_saveDirty = 0u;
    SoftTimer_Stop(&s_saveTmr);

    LcdHal_SetContrast((uint8_t)(((uint16_t)s_param[PARAM_BRIGHT] * 255u) / 100u));
    LcdHal_SetInvert(0u);

#if SPLASH_ENABLE
    s_splash = 1u;
    s_splashEnd = (uint16_t)(Timer_GetTick() + (uint16_t)SPLASH_TIME_MS);
    s_splashTick = Timer_GetTick();
    DrawSplash(0u);
#else
    s_splash = 0u;
#endif
}

/* 10ms 周期：软时钟、采样、自检推进、熄屏、延迟保存 */
void App_Task(void)
{
    uint16_t period;

    /* 软时钟 */
    s_rtcAcc += (uint16_t)APP_TASK_PERIOD_MS;
    if (s_rtcAcc >= 1000u) {
        s_rtcAcc -= 1000u;
        s_upSec++;
        if (++s_rtcS >= 60u) {
            s_rtcS = 0u;
            if (++s_rtcM >= 60u) {
                s_rtcM = 0u;
                if (++s_rtcH >= 24u) {
                    s_rtcH = 0u;
                }
                App_ParamSet(PARAM_HOUR, (int16_t)s_rtcH);
            }
            App_ParamSet(PARAM_MIN, (int16_t)s_rtcM);
        }
    }

    /* 采样：按采样周期推进，一次只做一步 */
    period = (uint16_t)s_param[PARAM_PERIOD];
    if (s_waveRun && Timer_Beat(&s_waveTick, period)) {
        s_phase = (uint8_t)((s_phase + 1u) % 32u);
        s_wave[s_waveHead] = s_sine[s_phase];
        s_waveHead = (uint8_t)((s_waveHead + 1u) % WAVE_POINTS);
    }

    /* 自检状态机 */
    if (s_testRun && SoftTimer_Expired(&s_testTmr)) {
        s_testStep++;
        if (s_testStep >= TEST_STEPS) {
            s_testRun = 0u;
        } else {
            SoftTimer_Start(&s_testTmr, 400u);
        }
    }

    /* 无操作熄屏 */
#if SLEEP_ENABLE
    if ((!s_sleep) && ((uint16_t)(Key_IdleMs() / 1000u) > (uint16_t)s_param[PARAM_SLEEP])) {
        s_sleep = 1u;
        LcdHal_DisplayOn(0u);
    }
#endif

    /* 参数延迟写回：真正写 Flash 的动作交给底层，绝不在菜单里等待 */
#if PARAM_SAVE_ENABLE
    if (s_saveDirty && SoftTimer_Expired(&s_saveTmr)) {
        s_saveDirty = 0u;
        Nvm_SaveAll((const uint8_t XRAM *)s_param, sizeof(s_param));
    }
#endif
}
