# gaokun3 图形安装器
#
#   make            宿主构建（只有 PNG 离线渲染）—— 不需要目标硬件就能验排版
#   make drm        设备构建（DRM + libinput），在 Alpine chroot 里编
#
# 依赖：宿主 = cairo pango；设备再加 libdrm libinput
CC      ?= cc
PKG     ?= pkg-config
CFLAGS  += -O2 -Wall -Wextra -Wno-unused-parameter -std=gnu11
SRC      = gk3-installer.c

HOST_CFLAGS = $(shell $(PKG) --cflags cairo pangocairo)
HOST_LIBS   = $(shell $(PKG) --libs   cairo pangocairo) -lm
DRM_CFLAGS  = $(shell $(PKG) --cflags libdrm libinput libudev)
DRM_LIBS    = $(shell $(PKG) --libs   libdrm libinput libudev)

all: gk3-installer-host

gk3-installer-host: $(SRC)
	$(CC) $(CFLAGS) $(HOST_CFLAGS) -o $@ $(SRC) $(HOST_LIBS)

drm: gk3-installer

gk3-installer: $(SRC)
	$(CC) $(CFLAGS) -DGK3_DRM $(HOST_CFLAGS) $(DRM_CFLAGS) -o $@ $(SRC) $(HOST_LIBS) $(DRM_LIBS)

# 离线看每一屏长什么样
shots: gk3-installer-host
	mkdir -p shots && ./gk3-installer-host --png-dir shots

clean:
	rm -f gk3-installer gk3-installer-host
	rm -rf shots

.PHONY: all drm shots clean
