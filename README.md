# STC8H · 128×64 单色点阵液晶 GUI 工程

面向 STC8H 的 128×64 单色点阵屏 GUI 框架：**三层菜单 + 全程非阻塞调度**，SDCC 与 Keil C51 两套编译器共用同一套源码。

## 特性

- **真非阻塞**：主循环内没有任何 `while 等待` / 软件延时；所有延时靠 1ms 时基比较
- **三层菜单**：列表 / 列表 / 叶子页（参数编辑、动作确认、自定义视图），数据驱动，改菜单只动一张表
- **分层清晰**：`Bsp`（时基、按键、屏驱）→ `GUI`（显存+图元+字库）→ `App`（菜单引擎+业务）
- **脏页分帧刷新**：绘制只改显存，每 3ms 最多搬 2 页，整屏传输摊到多帧，CPU 占用可控
- **中英混排**：ASCII 5×7（可选 2 倍放大）+ 汉字 16×16，字库由脚本离线生成
- **双编译器**：`Type.h` 自动切换 `code/xdata/data` 与 `interrupt` 语法（SDCC / Keil C51）

---

## 1. 快速开始

```powershell
# 方式一：一键脚本（推荐）
.\build.bat                # 编译 → output\gui.hex

# 方式二：make
cd source
make                       # 输出 ../output/gui.ihx 与 gui.hex

# 用 STC-ISP 下载 output\gui.hex（IRC 频率选 24MHz，见“常见问题”）
```

前置条件：[SDCC](https://sdcc.sourceforge.net/)（含 `sdcc`、`packihx`）；可选 `make`、`python`（仅生成字库时需要）。

脚本会**自动查找 sdcc**，依次尝试：PATH → `SDCC_HOME\bin` → `C:\Program Files\SDCC\bin` → VSCode 插件 Embedded IDE 自带工具链（`%USERPROFILE%\.eide\tools\...`）。找到后会打印 `[build] sdcc: <路径>` 并在本次会话中把它加入 PATH，因此**即使没配置 PATH 也能直接编译**。

若仍然报 `sdcc not found`，任选其一：

```powershell
# 方式 A：永久加入 PATH（改成你的实际路径）
setx PATH "$env:PATH;C:\Program Files\SDCC\bin"

# 方式 B：只设置根目录，脚本自动拼接 \bin
setx SDCC_HOME "C:\Program Files\SDCC"

# 方式 C：仅当前终端生效
$env:PATH = "C:\Program Files\SDCC\bin;$env:PATH"
```

> 本机目前没有系统级 SDCC，只有 Embedded IDE 自带的 `C:\Users\Y5043\.eide\tools\sdcc\bin`（4.2.0，支持 mcs51），脚本已能自动识别。

---

## 2. 目录结构

```
GUI/
├─ build.bat / build.ps1        一键编译脚本
├─ README.md                    本文件
├─ output/                      编译产物：gui.hex（烧录用）、gui.ihx、gui.map、gui.mem
└─ source/
   ├─ Type.h                   编译器抽象层（SDCC / Keil 自动适配）
   ├─ Config.h                 硬件与功能配置（引脚、屏型、时钟、开关）
   ├─ main.c                   主调度器
   ├─ Makefile                 make 构建
   ├─ Bsp/
   │   ├─ STC8H.h              精简寄存器定义
   │   ├─ Timer.c/.h           Timer0 1ms 时基 + 软件定时器
   │   ├─ Key.c/.h             5 键状态机（消抖/长按/连发）+ 事件队列
   │   └─ LcdHal.c/.h          SSD1306 / ST7565 页驱动（硬件 SPI）
   ├─ GUI/
   │   ├─ GUI.c/.h             1KB 显存、图元、文本、脏页刷新任务
   │   ├─ Font.c/.h            汉字二分检索 + 字库索引
   │   ├─ FontAscii.c/.h       95 个 ASCII 5×7（脚本生成，勿手改）
   │   └─ FontCN.c/.h          130 个汉字 16×16（脚本生成，勿手改）
   ├─ App/
   │   ├─ Menu.c/.h            三层菜单引擎
   │   ├─ MenuRes.c/.h         菜单数据表（改菜单只改这里）
   │   └─ App.c/.h             参数、视图页、动作、软时钟、熄屏、开机画面
   └─ Tools/gen_font.py        中文字模生成工具
```

---

## 3. 硬件连接

默认配置见 `source/Config.h`，改动只需编辑该文件。

### 3.1 屏幕（硬件 SPI，SSD1306 例）

| LCD 引脚 | STC8H | 说明 |
|---|---|---|
| CS | P1.2 | 片选，低有效 |
| DC | P3.2 | 数据/命令 |
| RES | P3.3 | 复位，低有效 |
| SCLK | P1.5 | SPI 时钟（硬件 SPI 默认组） |
| MOSI(SDA) | P1.3 | SPI 数据 |
| VCC / GND | 3.3V / GND | OLED 模块请注意电平 |

SPI 引脚组由 `LCD_SPI_SW`（`P_SW1` 的 `SPI_S[1:0]`）切换，默认 0（P1.2/P1.3/P1.4/P1.5）。

### 3.2 按键（低电平有效，内部上拉）

| 按键 | 引脚 |
|---|---|
| 上 | P0.0 |
| 下 | P0.1 |
| 左 | P0.2 |
| 右 | P0.3 |
| 确定 | P0.4 |

按键另一端接地；若你的板子是**高电平有效**，改 `KEY_ACTIVE_LEVEL` 为 1。

### 3.3 下载

P3.0/P3.1 接 USB-TTL，用 STC-ISP 下载 `output/gui.hex`。

---

## 4. 编译

### 4.1 一键脚本

```powershell
.\build.bat                # 编译 → output\gui.hex
.\build.bat rebuild        # 清理后全量编译
.\build.bat clean          # 只删中间文件，保留固件
.\build.bat distclean      # 连固件一起删
.\build.bat font           # 重新生成汉字 + ASCII 字库（需 python）
.\build.bat size           # 查看资源占用
```

`build.bat` 转发给 `build.ps1`（逻辑只在一处；脚本为纯 ASCII，避免 PowerShell 5.1 按 ANSI 解析中文出错）。脚本优先调用 `make`；找不到 `make` 时自动逐文件调用 `sdcc` 并手工链接，因此**只装 SDCC 也能编译**（`.\build.ps1 -NoMake` 可强制走这条路径）。

失败时会打印 `[build] FAILED: 原因`，并且 `build.bat` 会暂停窗口（双击运行时不会一闪而过）；`sdcc` / `python` 不在 PATH 时会给出明确提示。

### 4.2 make

```bash
cd source
make            # 编译
make rebuild    # 全量重编
make font       # 重新生成中文字库（Windows + Python + GDI）
make clean      # 只删中间文件
make distclean  # 连固件一起删
make size       # 打印资源占用
```

关键编译参数：`--model-large`（变量默认放 xdata）、`--iram-size 256`、`--xram-size 8192`、`--code-size 65536`。

### 4.3 Keil C51

1. 新建工程，器件选 STC8H 系列（或 Generic 8052 亦可编译）；
2. 把 `source` 下**全部 `.c`** 加入工程（`.h` 不必加）；
3. Options → C51 → **Memory Model: Large**（变量在 XDATA）；
4. Options → C51 → Include Paths 添加 `source` 目录；
5. Options → Output 勾选 **Create HEX File**，编译得 `.hex` 直接烧录。

`Type.h` 会自动把 `ROM/XRAM/IRAM` 映射成 Keil 的 `code/xdata/data`，`INTERRUPT_AT()` 映射成 `interrupt n using 1`，无需改动业务代码。

### 4.4 字库（汉字 + ASCII，均由脚本生成）

```bash
cd source && make font                        # 同时生成汉字与 ASCII 两套字库
python Tools/gen_font.py                      # 只生成汉字 16x16
python Tools/gen_font.py --ascii              # 只生成 ASCII 5x7
python Tools/gen_font.py --preview "--only=菜显熄"   # 预览汉字点阵
python Tools/gen_font.py --ascii --preview "--only=A0@"
```

- 汉字：编辑 `Tools/gen_font.py` 里的 `UI_TEXT`（工程用到的所有汉字集合）后重跑；
  渲染方式 = 宋体 32px → 裁剪包围盒 → 区域平均缩放到 16×16，笔画不易断裂。
- ASCII：`Lucida Console` 10px 渲染后直接取 5×7 包围盒，无缩放，字形规整。
- 生成物：`GUI/FontCN.c/.h`、`GUI/FontAscii.c/.h`（不要手工修改）。

---

## 5. 菜单与操作

### 5.1 三层结构

| 层级 | 内容 |
|---|---|
| L1 主菜单 | 系统设置 / 显示设置 / 数据采集 / 关于设备 |
| L2 分组 | 系统设置：小时设置、分钟设置、恢复出厂、设备信息<br>显示设置：亮度调节、熄屏时间、反色显示、界面语言<br>数据采集：采样周期、通道选择、启动采样、存储管理<br>关于设备：固件版本、编译时间、硬件自检、运行时间 |
| L3 叶子页 | 参数编辑页 / 动作确认页 / 自定义视图页（波形、自检、版本、运行时间…） |

### 5.2 按键定义

| 按键 | 列表页 | 参数页 | 动作页 | 视图页 |
|---|---|---|---|---|
| 上 / 下 | 上一项 / 下一项（长按连滚） | ±10 步长 | — | 交给页面自处理 |
| 左 / 右 | 返回上一层 | ∓1 步长 | 切换 确认/取消 | 交给页面自处理 |
| 确定 OK | 进入下一层 / 打开叶子页 | 保存并返回 | 执行选中按钮 | 退出 |
| **长按 OK** | 返回上层 | **放弃修改**返回 | 取消 | 退出 |

### 5.3 页面布局

```
┌──────────────────────────────┐ 0
│ 标题(16px)            12:34  │ 标题栏，右侧为状态区
├──────────────────────────────┤ 16
│                              │
│      正文 48px（3 行×16）     │
│                              │
└──────────────────────────────┘ 63
```

---

## 6. 非阻塞架构

### 6.1 任务节拍

| 任务 | 周期 | 职责 | 单次耗时 |
|---|---|---|---|
| `Key_Scan()` | 1ms | 按键状态机，结果进事件 FIFO | ≈10us |
| `App_Task()` | 10ms | 软时钟、采样推进、自检状态机、熄屏判断 | <0.5ms |
| `Menu_Task()` | 事件驱动 / 动画页 20ms | 状态变化或动画帧时重绘显存 | 1~5ms |
| `GUI_FlushTask()` | 3ms，每帧最多 2 页 | 显存 → 屏幕 | ≈0.34ms |

主循环只做“看一眼时基 → 干一点活 → 立刻返回”，中断里只 `s_tick++`。

### 6.2 三条实现纪律

1. **禁止阻塞延时**：需要等待一律用 `Timer_Beat()` / `SoftTimer_Expired()` 比较时基；
2. **长任务分步**：硬件自检 = 4 步状态机，每步 400ms；采样按周期推进一次一步；
3. **Flash 写入延后**：参数改完只置脏标记 + 软定时器，到点再写，绝不在菜单响应里等待擦写。

### 6.3 关键接口

```c
Timer_GetTick();                 // 读 1ms 时基（中断安全）
Timer_Beat(&tick, period);       // 周期到？到则推进 tick 并返回 1
SoftTimer_Start(&t, ms);
SoftTimer_Expired(&t);
Key_Get(&msg);                   // 取一条按键事件，无事件返回 0
Menu_Key(msg);                   // 把事件交给菜单
Menu_Refresh();                  // 请求重绘
Menu_Toast("已保存", 1000);      // 非阻塞提示条
```

---

## 7. 二次开发

### 7.1 加/改菜单项

只改 `App/MenuRes.c`。叶子页三种类型：

```c
{"小时设置", MENU_ITEM_PARAM,  PARAM_HOUR,    NULL},   // 参数编辑页
{"恢复出厂", MENU_ITEM_ACTION, ACTION_RESET,  NULL},   // 动作确认页
{"设备信息", MENU_ITEM_VIEW,   VIEW_INFO,     NULL},   // 自定义视图页
{"显示设置", MENU_ITEM_SUBMENU, 0u, &s_pageDisp},      // 下一层列表
```

菜单字符串直接写中文（UTF-8 源文件），**用到的汉字必须已收录在字库**（见 4.4）。

### 7.2 加一个参数

1. `MenuRes.h` 的 `ParamId_t` 增加枚举；
2. `App.c` 的 `s_def[]`（默认值）与 `s_info[]`（最小/最大/步长）各加一项；
3. `App_ParamFormat()` 加显示格式分支；需要立即生效的（如亮度改对比度寄存器）在 `App_ParamSet()` 里加 `case`；
4. `MenuRes.c` 里挂一个 `MENU_ITEM_PARAM` 菜单项。

### 7.3 加一个视图页（动画/长任务）

1. `MenuRes.h` 的 `ViewId_t` 增加枚举，`MenuRes.c` 挂 `MENU_ITEM_VIEW`；
2. `App.c` 实现：`App_ViewEnter()`（进入时初始化）、`App_ViewDraw()`（只画正文区 y=16~63）、`App_ViewKey()`（页面内按键）、`App_ViewNeedDraw()`（返回 1 表示动画页，按 20ms 重绘）；
3. 耗时操作写成状态机，由 `App_Task()`（10ms）分步推进，`App_ViewDraw()` 只负责“画当前状态”。

### 7.4 换屏 / 换引脚 / 换时钟

| 需求 | 改动位置 |
|---|---|
| 换屏（SSD1306 ↔ ST7565） | `Config.h` 的 `LCD_CTRL` |
| 换引脚 | `Config.h` 的 `SBIT_AT` 行与 `LCD_SPI_SW` |
| 换主频 | `Config.h` 的 `FOSC` + STC-ISP 下载时 IRC 频率（两者必须一致） |
| 刷新更快/更省电 | `GUI_FLUSH_PERIOD_MS`、`GUI_FLUSH_PAGES`、`GUI_RENDER_PERIOD_MS` |
| 关掉开机画面/熄屏/看门狗 | `SPLASH_ENABLE`、`SLEEP_ENABLE`、`WDT_ENABLE` |

### 7.5 接入真实数据

- **ADC 采样**：`ADC_ENABLE=1`，在 `App.c` 用非阻塞方式启转换、到点读结果替换正弦表；
- **掉电保存**：`PARAM_SAVE_ENABLE=1` 并实现 `Nvm_SaveAll()`（IAP 或外挂 EEPROM 皆可，注意放在 `App_Task()` 里分步执行）。

---

## 8. 资源占用（SDCC 4.x 实测）

| 项目 | 用量 | 余量 |
|---|---|---|
| FLASH | 20,253 B | / 64 KB（含 130 汉字 + 95 ASCII 字库） |
| XRAM | 1,590 B | / 8 KB（其中显存 1,024 B） |
| 内部 RAM | 约 72 B 数据 + 184 B 栈 | / 256 B |

`build.bat size` 或 `make size` 可随时查看最新占用（读 `output/gui.mem`）。

---

## 9. 常见问题

| 现象 | 排查 |
|---|---|
| 白屏 / 无显示 | ①`LCD_CTRL` 型号对不对；②DC/RES/CS 引脚是否与 `Config.h` 一致；③模块供电与电平；④换 SPI 引脚组改 `LCD_SPI_SW` |
| 显示上下/左右错位 | 改 `LcdHal.c` 初始化里的 `0xA1/0xA0`（段方向）、`0xC0/0xC8`（COM 方向）、`0xDA`（COM 引脚配置） |
| 中文显示成空白 | 该汉字未收录：加入 `Tools/gen_font.py` 的 `UI_TEXT` 后 `make font` |
| 时间/节奏不准 | `FOSC` 与 STC-ISP 下载时选择的 IRC 频率不一致（默认 24MHz） |
| 按键无反应或反了 | `KEY_ACTIVE_LEVEL`；端口是否设为准双向（内部上拉未开需外接上拉） |
| 菜单卡顿 | 调大 `GUI_FLUSH_PERIOD_MS` 或减小 `GUI_FLUSH_PAGES`；检查是否在回调里加了阻塞等待 |
| Keil 编译报 `code/xdata` 错 | 确认 Memory Model = Large，Include Paths 含 `source` 目录 |

---

## 10. 许可

示例工程，可自由使用与修改；中文字模由系统宋体渲染生成，商用请注意字体授权。
