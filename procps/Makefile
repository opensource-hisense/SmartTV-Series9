
CROSS_COMPILE ?= /mtkoss/gnuarm/neon_4.8.2_2.6.35_cortex-a9-ubuntu/x86_64/bin/armv7a-mediatek482_001_neon-linux-gnueabi-
PLATFORM_ARCH ?= armv7a-mediatek-linux-gnueabi
OSS_LIB_ROOT  ?=
OSS_OUTPUT    ?=
NCURSES_VER   ?= ncurses-5.7
PROCPS_VER    ?= procps-3.2.8

MODULE_VER  := $(PROCPS_VER)
MODULE_NAME := procps


NCURSES_INSTALL_PATH := $(OSS_LIB_ROOT)/ncurses/$(NCURSES_VER)

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
	make -C ${MODULE_SRC_BUILD_PATH} SHARED=0 DESTDIR=${MODULE_INSTALL_PATH} NCURSES_PATH=${NCURSES_INSTALL_PATH}

install:
	mkdir -p ${MODULE_INSTALL_PATH}/lib
	mkdir -p ${MODULE_INSTALL_PATH}/bin
	make -C ${MODULE_SRC_BUILD_PATH} SHARED=0 DESTDIR=${MODULE_INSTALL_PATH} NCURSES_PATH=${NCURSES_INSTALL_PATH} install

clean:
	rm -rf $(MODULE_SRC_BUILD_PATH)
	rm -rf $(MODULE_INSTALL_PATH)

prepare:
	if [ ! -d $(MODULE_OUTPUT) ]; then \
		mkdir -p $(MODULE_OUTPUT); \
	fi
	tar -zxvf $(MODULE_TAR) -C $(MODULE_OUTPUT)
	cp -rf $(MODULE_VER)_patch/* $(MODULE_SRC_BUILD_PATH)/


notice:
	cp -rf NOTICE.txt $(OSS_NOTICE_OUTPUT)/$(MODULE_NAME)_NOTICE.txt

info:
	cp -rf OSS_INFO.txt $(OSS_INFO_OUTPUT)/$(MODULE_NAME)_INFO.txt

