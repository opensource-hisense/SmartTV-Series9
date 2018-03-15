
CROSS_COMPILE ?= /mtkoss/gnuarm/neon_4.8.2_2.6.35_cortex-a9-ubuntu/x86_64/bin/armv7a-mediatek482_001_neon-linux-gnueabi-
PLATFORM_ARCH ?= armv7a-mediatek-linux-gnueabi
OSS_LIB_ROOT  ?=
OSS_OUTPUT    ?=
COREUTIL_VER  ?= coreutils-6.9

MODULE_VER  := $(COREUTIL_VER)
MODULE_NAME := coreutils


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
	./configure --prefix=${MODULE_INSTALL_PATH} --host=armv7a-mediatek451_001_vfp-linux-gnueabi --enable-install-program=df
	make -C ${MODULE_SRC_BUILD_PATH}

install:
	make -C ${MODULE_SRC_BUILD_PATH} install
	if [ ! -f ${MODULE_INSTALL_PATH}/bin/su ] ; then \
		cp ${MODULE_SRC_BUILD_PATH}/src/su ${MODULE_INSTALL_PATH}/bin/ ; \
	fi

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

