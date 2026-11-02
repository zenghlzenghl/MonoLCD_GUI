# =============================================================================
# 128x64 单色点阵 GUI 工程（MCU: STC8H）
#
# 编译：make            # 输出 output/gui.ihx / gui.hex(需 packihx)
#       make rebuild    # 全量重编
#       make clean      # 只删中间文件，保留固件
#       make distclean  # 连固件一起删
#       make font       # 重新生成中文字库(需要 Windows + Python)
#       make size       # 查看资源占用
#
# 说明：
#   1. 默认使用 SDCC(mcs51)，Keil C51 用户把全部 .c/.h 加入工程即可，
#      Type.h 会自动切换到 code/xdata/data 关键字与 interrupt 语法；
#   2. FOSC 必须与 STC-ISP 下载时选择的 IRC 频率一致（见 Config.h）；
#   3. 换屏只需改 Config.h 中的 LCD_CTRL（SSD1306 / ST7565）。
# =============================================================================

CC      = sdcc
PACKIHX = packihx
OUTDIR  = ../output
TARGET  = gui

SRCS = \
	main.c \
	Bsp/Timer.c \
	Bsp/Key.c \
	Bsp/LcdHal.c \
	GUI/GUI.c \
	GUI/Font.c \
	GUI/FontAscii.c \
	GUI/FontCN.c \
	App/Menu.c \
	App/MenuRes.c \
	App/App.c

OBJS = $(SRCS:.c=.rel)

# large 模型：变量默认放 xdata（STC8H 内部扩展 RAM）
CFLAGS  = -mmcs51 --model-large --opt-code-speed \
          --iram-size 256 --xram-size 8192 --code-size 65536 \
          -I. -I..
LDFLAGS = -mmcs51 --model-large --iram-size 256 --xram-size 8192 --code-size 65536

all: $(OUTDIR)/$(TARGET).ihx size

rebuild: distclean all

size:
	@echo ""
	@echo "---- 资源占用 ----"
	@cat $(OUTDIR)/$(TARGET).mem 2>/dev/null || echo "(先执行 make 生成 .mem)"

$(OUTDIR)/$(TARGET).ihx: $(OBJS)
	mkdir -p $(OUTDIR)
	$(CC) $(LDFLAGS) -o $(OUTDIR)/$(TARGET).ihx $(OBJS)
	-$(PACKIHX) $(OUTDIR)/$(TARGET).ihx > $(OUTDIR)/$(TARGET).hex 2>&1

%.rel: %.c
	$(CC) $(CFLAGS) -c $< -o $@

font:
	python Tools/gen_font.py
	python Tools/gen_font.py --ascii

clean:                                  # 只清理中间文件，保留 output 里的固件
	rm -f $(SRCS:.c=.rel) $(SRCS:.c=.sym) $(SRCS:.c=.asm) $(SRCS:.c=.lst) $(SRCS:.c=.rst)

distclean: clean                        # 连固件一起清理
	rm -f $(OUTDIR)/$(TARGET).ihx $(OUTDIR)/$(TARGET).hex $(OUTDIR)/$(TARGET).map $(OUTDIR)/$(TARGET).lk $(OUTDIR)/$(TARGET).mem

.PHONY: all rebuild size clean distclean font
