# -*- coding: utf-8 -*-
"""
离线字模生成工具（Windows GDI 渲染，生成后无需依赖任何第三方库）

用法：
    python Tools/gen_font.py                      生成 GUI/FontCN.c/.h（16x16 汉字）
    python Tools/gen_font.py --ascii              生成 GUI/FontAscii.c/.h（5x7 ASCII）
    python Tools/gen_font.py --preview "--only=菜显熄"

生成规则：
    汉字 16x16：逐行式，每行 2 字节，bit7 对应最左像素（与 SSD1306 页顺序无关，GUI 层逐点绘制）
    ASCII 5x7 ：逐行式，每行 1 字节，bit7 对应 x0，字符宽 5 + 1 列间距
"""

import ctypes
import os
import sys
from ctypes import POINTER, byref, c_int, c_void_p, wintypes

#---------------------------------------------------------------- 配置
OUT_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "GUI")

CN_FACE = "SimSun"          # 宋体
CN_PX   = 32                # 大字号渲染后缩放，点阵更清晰
CN_W    = 16
CN_H    = 16

AS_FACE = "Lucida Console"  # 10px 下正好是 5x7 点阵，无需缩放
AS_PX   = 10
AS_W    = 5
AS_H    = 7

# 工程里出现过的全部汉字（改菜单/加页面后把新字加进来再重新生成）
UI_TEXT = (
    "主菜单系统设置显示数据采集关于设备时间语言恢复出厂信息"
    "亮度调节对比度熄屏反色采样周期通道选择启动存储管理"
    "固件版本编译硬件自检返回上层小时分钟确认取消开关中英毫秒秒"
    "擦除记录剩余容量电压温度状态成功失败进行完成按住编辑保存已请稍候"
    "空闲运行停止波形实时曲线峰值平均参数值设备界面导航上下左右确定退出"
    "操作文键自检完成"
)

PREVIEW = "--preview" in sys.argv
ONLY = ""
for _a in sys.argv[1:]:
    if _a.startswith("--only="):
        ONLY = _a[7:]

#---------------------------------------------------------------- GDI
class RECT(ctypes.Structure):
    _fields_ = [("left", c_int), ("top", c_int), ("right", c_int), ("bottom", c_int)]


class BITMAPINFOHEADER(ctypes.Structure):
    _fields_ = [
        ("biSize", wintypes.DWORD), ("biWidth", c_int), ("biHeight", c_int),
        ("biPlanes", wintypes.WORD), ("biBitCount", wintypes.WORD),
        ("biCompression", wintypes.DWORD), ("biSizeImage", wintypes.DWORD),
        ("biXPelsPerMeter", c_int), ("biYPelsPerMeter", c_int),
        ("biClrUsed", wintypes.DWORD), ("biClrImportant", wintypes.DWORD),
    ]


class BITMAPINFO(ctypes.Structure):
    _fields_ = [("bmiHeader", BITMAPINFOHEADER), ("bmiColors", wintypes.DWORD * 3)]


gdi32   = ctypes.WinDLL("gdi32", use_last_error=True)
user32  = ctypes.WinDLL("user32", use_last_error=True)

user32.FillRect.argtypes = [wintypes.HDC, POINTER(RECT), wintypes.HANDLE]
user32.FillRect.restype = c_int
gdi32.CreateDIBSection.argtypes = [wintypes.HDC, POINTER(BITMAPINFO), wintypes.UINT,
                                   POINTER(c_void_p), wintypes.HANDLE, wintypes.DWORD]
gdi32.CreateDIBSection.restype = wintypes.HANDLE
gdi32.SelectObject.argtypes = [wintypes.HDC, wintypes.HANDLE]
gdi32.SelectObject.restype = wintypes.HANDLE
gdi32.DeleteObject.argtypes = [wintypes.HANDLE]
gdi32.DeleteObject.restype = wintypes.BOOL
gdi32.CreateFontW.argtypes = [c_int, c_int, c_int, c_int, c_int, wintypes.DWORD,
                              wintypes.DWORD, wintypes.DWORD, wintypes.DWORD,
                              wintypes.DWORD, wintypes.DWORD, wintypes.DWORD,
                              wintypes.DWORD, wintypes.LPCWSTR]
gdi32.CreateFontW.restype = wintypes.HANDLE
gdi32.TextOutW.argtypes = [wintypes.HDC, c_int, c_int, wintypes.LPCWSTR, c_int]
gdi32.TextOutW.restype = wintypes.BOOL


def render(ch, face, font_px, cell_w, cell_h, fit=False):
    """渲染字符到 cell_w x cell_h 点阵
    fit=False: 大字号渲染 -> 裁剪包围盒 -> 区域平均缩放填满 cell（汉字）
    fit=True : 字号已接近 cell，直接裁剪包围盒放入 cell（ASCII 5x7）
    """
    W, H = cell_w * 4, cell_h * 4

    hdc = gdi32.CreateCompatibleDC(0)
    bmi = BITMAPINFO()
    bmi.bmiHeader.biSize = ctypes.sizeof(BITMAPINFOHEADER)
    bmi.bmiHeader.biWidth = W
    bmi.bmiHeader.biHeight = -H          # top-down
    bmi.bmiHeader.biPlanes = 1
    bmi.bmiHeader.biBitCount = 32
    bmi.bmiHeader.biCompression = 0

    bits = c_void_p()
    hbmp = gdi32.CreateDIBSection(hdc, byref(bmi), 0, byref(bits), None, 0)
    old = gdi32.SelectObject(hdc, hbmp)

    rc = RECT(0, 0, W, H)
    brush = gdi32.CreateSolidBrush(0x00FFFFFF)
    user32.FillRect(hdc, byref(rc), brush)
    gdi32.DeleteObject(brush)

    # 134=GB2312_CHARSET, 4=ANTIALIASED_QUALITY(细笔画不丢), 0x30=固定间距+MODERN
    hfont = gdi32.CreateFontW(-font_px, 0, 0, 0, 400, 0, 0, 0,
                              134, 0, 0, 4, 0x30, face)
    gdi32.SelectObject(hdc, hfont)
    gdi32.SetTextColor(hdc, 0x00000000)
    gdi32.SetBkMode(hdc, 1)              # TRANSPARENT
    gdi32.TextOutW(hdc, 0, 0, ch, 1)

    arr = (ctypes.c_ubyte * (W * H * 4)).from_address(bits.value)
    grid = [[1 if arr[(y * W + x) * 4] < 160 else 0 for x in range(W)] for y in range(H)]

    gdi32.SelectObject(hdc, old)
    gdi32.DeleteObject(hfont)
    gdi32.DeleteObject(hbmp)
    gdi32.DeleteDC(hdc)

    xs = [x for y in range(H) for x in range(W) if grid[y][x]]
    if not xs:
        return [[0] * cell_w for _ in range(cell_h)]
    ys = [y for y in range(H) for x in range(W) if grid[y][x]]
    x0, x1, y0, y1 = min(xs), max(xs), min(ys), max(ys)
    gw, gh = x1 - x0 + 1, y1 - y0 + 1
    out = [[0] * cell_w for _ in range(cell_h)]

    if fit:
        take_w, take_h = min(gw, cell_w), min(gh, cell_h)
        sx0 = x0 + (gw - take_w) // 2
        sy0 = y0 + (gh - take_h)              # 底对齐：超出时裁掉顶部
        ox = (cell_w - take_w) // 2
        oy = cell_h - take_h
        for j in range(take_h):
            for i in range(take_w):
                if grid[sy0 + j][sx0 + i]:
                    out[oy + j][ox + i] = 1
        return out

    # 区域平均缩放：源包围盒 -> 目标 cell（填满，保证小字号下笔画不断裂）
    for dy in range(cell_h):
        sy0 = y0 + dy * gh // cell_h
        sy1 = y0 + max((dy + 1) * gh // cell_h, (dy * gh // cell_h) + 1)
        if sy1 > y1 + 1:
            sy1 = y1 + 1
        for dx in range(cell_w):
            sx0 = x0 + dx * gw // cell_w
            sx1 = x0 + max((dx + 1) * gw // cell_w, (dx * gw // cell_w) + 1)
            if sx1 > x1 + 1:
                sx1 = x1 + 1
            tot = (sy1 - sy0) * (sx1 - sx0)
            cnt = sum(grid[j][i] for j in range(sy0, sy1) for i in range(sx0, sx1))
            out[dy][dx] = 1 if (cnt * 2 >= tot) else 0
    return out


def row_byte(row):
    """一行打包成 1 字节（或两字节），bit7 对应最左像素"""
    v = 0
    for i, b in enumerate(row[:8]):
        if b:
            v |= (0x80 >> i)
    return v


def pack(g, cell_w, cell_h):
    """逐行式：每行 ceil(cell_w/8) 字节"""
    out = []
    for y in range(cell_h):
        row = g[y]
        for k in range((cell_w + 7) // 8):
            out.append(row_byte(row[k * 8:(k + 1) * 8]))
    return out


def hexs(data, per_line):
    return "\n".join("    " + " ".join("0x%02X," % b for b in data[i:i + per_line])
                     for i in range(0, len(data), per_line))


#---------------------------------------------------------------- 输出模板
HEADER_CN_C = """/* 本文件由 Tools/gen_font.py 自动生成，请勿手工修改
 * 字模: 宋体 16x16，逐行式，每行 2 字节，高位在左
 * 汉字数量: %d
 * 字符表:
 *   %s
 */
#include "FontCN.h"

const uint8_t ROM FontCn16Data[FONT_CN16_COUNT][32] = {
%s
};

/* UTF-8 三字节检索表（与 FontCn16Data 顺序一一对应，供二分查找） */
const uint8_t ROM FontCnKey[FONT_CN16_COUNT][3] = {
%s
};
"""

HEADER_CN_H = """#ifndef __FONT_CN_H
#define __FONT_CN_H

#include "Type.h"

#define FONT_CN16_COUNT  %du
#define FONT_CN_SIZE     3u      /* UTF-8: 一个汉字 3 字节 */

extern const uint8_t ROM FontCn16Data[FONT_CN16_COUNT][32];
extern const uint8_t ROM FontCnKey[FONT_CN16_COUNT][3];

#endif
"""

HEADER_AS_C = """/* 本文件由 Tools/gen_font.py --ascii 自动生成，请勿手工修改
 * 字模: 5x7 ASCII（0x20~0x7E），逐行式，每行 1 字节，bit7 对应最左像素
 */
#include "FontAscii.h"

const uint8_t ROM FontAscii5x7[FONT_ASCII_COUNT][7] = {
%s
};
"""

HEADER_AS_H = """#ifndef __FONT_ASCII_H
#define __FONT_ASCII_H

#include "Type.h"

#define FONT_ASCII_FIRST  0x20u
#define FONT_ASCII_LAST   0x7Eu
#define FONT_ASCII_COUNT  (FONT_ASCII_LAST - FONT_ASCII_FIRST + 1u)
#define FONT_ASCII_W      5u      /* 点阵宽 */
#define FONT_ASCII_H      7u      /* 点阵高 */

extern const uint8_t ROM FontAscii5x7[FONT_ASCII_COUNT][7];

#endif
"""


def write(name, text):
    path = os.path.normpath(os.path.join(OUT_DIR, name))
    with open(path, "w", encoding="utf-8") as f:
        f.write(text)
    print("generated: %s" % path)


#---------------------------------------------------------------- 主流程
def gen_cn():
    chars = []
    for ch in (ONLY if ONLY else UI_TEXT):
        if ch not in chars:
            chars.append(ch)
    chars.sort(key=lambda c: c.encode("utf-8"))

    if PREVIEW:
        for ch in chars:
            g = render(ch, CN_FACE, CN_PX, CN_W, CN_H)
            print("\nU+%04X  %s  16x16" % (ord(ch), ch))
            for y in range(CN_H):
                print("  " + "".join("#" if g[y][x] else "." for x in range(CN_W)))
        return

    data = [pack(render(ch, CN_FACE, CN_PX, CN_W, CN_H), CN_W, CN_H) for ch in chars]
    body = "\n".join("  {\n%s\n  }," % hexs(d, 16) for d in data)
    keys = "\n".join("  {0x%02X, 0x%02X, 0x%02X}," % tuple(ch.encode("utf-8")) for ch in chars)

    write("FontCN.c", HEADER_CN_C % (len(chars), "".join(chars), body, keys))
    write("FontCN.h", HEADER_CN_H % len(chars))
    print("CN glyphs: %d" % len(chars))


def gen_ascii():
    chars = [chr(c) for c in range(0x20, 0x7F)]

    if PREVIEW:
        for ch in ONLY or "Az0@":
            g = render(ch, AS_FACE, AS_PX, AS_W, AS_H, True)
            print("\n'%s'  5x7" % ch)
            for y in range(AS_H):
                print("  " + "".join("#" if g[y][x] else "." for x in range(AS_W)))
        return

    data = [pack(render(ch, AS_FACE, AS_PX, AS_W, AS_H, True), AS_W, AS_H) for ch in chars]
    body = "".join("  /* '%s' */\n  {\n%s\n  },\n"
                   % ("\\" if c == "\\" else "'" if c == "'" else c, hexs(d, 8))
                   for c, d in zip(chars, data))
    write("FontAscii.c", HEADER_AS_C % body)
    write("FontAscii.h", HEADER_AS_H)
    print("ASCII glyphs: %d" % len(chars))


if __name__ == "__main__":
    gen_ascii() if "--ascii" in sys.argv else gen_cn()
