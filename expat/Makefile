
EXPAT_ROOT=$(shell pwd)

ifndef OSS_ROOT
OSS_ROOT ?= $(word 1, $(subst /oss/,/oss /, $(shell pwd)))
endif

ifndef THIRDPARTY_ROOT
THIRDPARTY_ROOT := $(OSS_ROOT)/../third_party
endif

ifndef DTV_LINUX_ROOT
DTV_LINUX_ROOT := $(OSS_ROOT)/../linux_mts
endif

ifndef DTV_LINUX_MAK_ROOT
DTV_LINUX_MAK_ROOT := $(DTV_LINUX_ROOT)/build/mak
endif
include $(DTV_LINUX_ROOT)/build/mak/common_inc.mak

EXPAT_OUTPUT := $(OSS_OUTPUT)/expat
EXPAT_SOURCE := $(EXPAT_OUTPUT)/expat-$(EXPAT_VERSION)
EXPAT_BUILD := $(EXPAT_OUTPUT)/expat-$(EXPAT_VERSION)-build
EXPAT_FILE := $(EXPAT_VERSION)/expat-$(EXPAT_VERSION).tar.gz
MTK_TOOLCHAIN_CC_FLAG += -D_CPU_LITTLE_ENDIAN -g -O2

all: expat_prepare
	if [ ! -d $(EXPAT_BUILD) ]; then \
		mkdir -p $(EXPAT_BUILD); \
	fi
	cd $(EXPAT_BUILD) && \
	CC="$(CROSS_COMPILE)gcc" RANLIB="$(CROSS_COMPILE)ranlib" CFLAGS="$(MTK_TOOLCHAIN_CC_FLAG)" \
	$(EXPAT_SOURCE)/configure \
	 --host=arm-mtk-linux \
	 --prefix=$(OSS_LIB_ROOT)/expat/$(EXPAT_VERSION)
	make -j $(JOBS) -C $(EXPAT_BUILD)

install:
	make -C $(EXPAT_BUILD) install
	
clean:
	rm -rf $(EXPAT_OUTPUT)

notice:
	cp -rf NOTICE.txt $(OSS_NOTICE_OUTPUT)/expat_NOTICE.txt
	
info:
	cp -rf OSS_INFO.$(EXPAT_VERSION).txt $(OSS_INFO_OUTPUT)/expat_INFO.txt

expat_prepare:
	if [ ! -d $(EXPAT_SOURCE) ]; then \
		mkdir -p $(EXPAT_SOURCE); \
		tar zxf $(EXPAT_FILE) -C $(EXPAT_OUTPUT); \
		chmod -R 755 $(EXPAT_SOURCE); \
	fi

