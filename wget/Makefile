WGET_OUTPUT := $(OSS_OUTPUT)/wget
WGET_SOURCE := $(WGET_OUTPUT)/wget-$(WGET_VERSION)
WGET_BUILD  := $(WGET_OUTPUT)/wget-$(WGET_VERSION)-build
WGET_FILE := wget-$(WGET_VERSION).tar.gz

LD_LIBRARY_PATH :=
JOBS ?= $(shell grep processor /proc/cpuinfo | wc -l)
WGET_CFLAGS := -O2

all: wget_prepare
	if [ ! -d $(WGET_BUILD) ]; then \
	    mkdir -p $(WGET_BUILD)	; \
	fi
	cd $(WGET_BUILD) && \
	CC="$(CROSS_COMPILE)gcc" CFLAGS="$(WGET_CFLAGS) $(PLATFORM_CFLAGS)" AR="$(CROSS_COMPILE)ar" RANLIB="$(CROSS_COMPILE)ranlib" \
	$(WGET_SOURCE)/configure \
		--prefix=$(OSS_LIB_ROOT)/wget/$(WGET_VERSION) \
		--target=arm-linux \
		--host=$(PLATFORM_ARCH)
	make -j $(JOBS) -C $(WGET_BUILD)

install:
	make -C $(WGET_BUILD) install

clean:
	rm -rf $(WGET_OUTPUT)

wget_prepare:
	if [ ! -d $(WGET_SOURCE) ]; then \
		mkdir -p $(WGET_SOURCE); \
		tar zxf $(WGET_FILE) -C $(WGET_OUTPUT); \
                chmod -R 755 ${WGET_SOURCE} ; \
	fi

notice:
	cp -rf NOTICE.txt $(OSS_NOTICE_OUTPUT)/wget_NOTICE.txt

info:
	cp -rf OSS_INFO.txt $(OSS_INFO_OUTPUT)/wget_INFO.txt

