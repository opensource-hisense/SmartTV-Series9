
CROSS_COMPILE ?= /mtkoss/gnuarm/neon_4.8.2_2.6.35_cortex-a9-ubuntu/x86_64/bin/armv7a-mediatek482_001_neon-linux-gnueabi-
PLATFORM_ARCH ?= armv7a-mediatek-linux-gnueabi
OSS_LIB_ROOT  ?=
OSS_OUTPUT    ?=
BASH_VER      ?= bash-3.2.48

MODULE_VER  := $(BASH_VER)
MODULE_NAME := bash


export CC=${CROSS_COMPILE}gcc
export AR=${CROSS_COMPILE}ar
export LD=${CROSS_COMPILE}ld
export STRIP=${CROSS_COMPILE}strip
export CXX=${CROSS_COMPILE}g++

CFLAGS += -D_GNU_SOURCE

MODULE_OUTPUT := $(OSS_OUTPUT)/$(MODULE_NAME)
MODULE_SRC_BUILD_PATH := $(MODULE_OUTPUT)/$(MODULE_VER)
MODULE_TAR := $(MODULE_VER).tar.gz
MODULE_INSTALL_PATH := $(OSS_LIB_ROOT)/$(MODULE_NAME)/$(MODULE_VER)


all: prepare
	cd ${MODULE_SRC_BUILD_PATH}; \
	./configure --prefix=${MODULE_INSTALL_PATH} --host=arm-linux
	make -C ${MODULE_SRC_BUILD_PATH}

install:
	make -C ${MODULE_SRC_BUILD_PATH} install

clean:
	rm -rf $(MODULE_SRC_BUILD_PATH)
	rm -rf $(MODULE_INSTALL_PATH)

prepare:
	if [ ! -d $(MODULE_OUTPUT) ]; then \
		mkdir -p $(MODULE_OUTPUT); \
	fi
	tar -zxvf $(MODULE_TAR) -C $(MODULE_OUTPUT)
	patch -d $(MODULE_SRC_BUILD_PATH) -p2 < ./bash-3.2-patches/bash32-049
	patch -d $(MODULE_SRC_BUILD_PATH) -p2 < ./bash-3.2-patches/bash32-050
	patch -d $(MODULE_SRC_BUILD_PATH) -p2 < ./bash-3.2-patches/bash32-051
	patch -d $(MODULE_SRC_BUILD_PATH) -p2 < ./bash-3.2-patches/bash32-052.txt
	patch -d $(MODULE_SRC_BUILD_PATH) -p2 < ./bash-3.2-patches/bash32-053.txt
	patch -d $(MODULE_SRC_BUILD_PATH) -p2 < ./bash-3.2-patches/bash32-054.txt
	patch -d $(MODULE_SRC_BUILD_PATH) -p2 < ./bash-3.2-patches/bash32-055
	patch -d $(MODULE_SRC_BUILD_PATH) -p2 < ./bash-3.2-patches/bash32-056
	patch -d $(MODULE_SRC_BUILD_PATH) -p2 < ./bash-3.2-patches/bash32-057
	patch -d $(MODULE_SRC_BUILD_PATH) -p2 < ./bash-3.2-patches/bash32-mtk-patch-001

notice:
	cp -rf NOTICE.txt $(OSS_NOTICE_OUTPUT)/$(MODULE_NAME)_NOTICE.txt

info:
	cp -rf OSS_INFO.txt $(OSS_INFO_OUTPUT)/$(MODULE_NAME)_INFO.txt

