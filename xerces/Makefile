
XERCES_VERSION := 3.1.1

XERCES_OUTPUT := $(OSS_OUTPUT)/xerces
XERCES_SOURCE := $(XERCES_OUTPUT)/xerces-c-$(XERCES_VERSION)
XERCES_BUILD  := $(XERCES_OUTPUT)/xerces-c-$(XERCES_VERSION)-build
XERCES_FILE := xerces-c-$(XERCES_VERSION).tar.bz2

LD_LIBRARY_PATH :=
JOBS ?= $(shell grep processor /proc/cpuinfo | wc -l)
XERCES_CFLAGS := -g -fPIC -O2 
	
all: xerces_prepare
	if [ ! -d $(XERCES_BUILD) ]; then \
	    mkdir -p $(XERCES_BUILD)	; \
	fi 
	cd $(XERCES_BUILD) && \
	CC="$(CROSS_COMPILE)gcc" CFLAGS="$(XERCES_CFLAGS) $(PLATFORM_CFLAGS)" AR="$(CROSS_COMPILE)ar rus" RANLIB="$(CROSS_COMPILE)ranlib" \
	LD="$(CROSS_COMPILE)ld" NM="$(CROSS_COMPILE)nm" CXX="$(CROSS_COMPILE)g++" $(XERCES_SOURCE)/configure \
		--prefix=$(OSS_LIB_ROOT)/xerces/$(XERCES_VERSION) \
        --build=i686-pc-linux       \
		--host=$(PLATFORM_ARCH)     \
        --disable-static            \
        --disable-network           \
        --disable-sse2              \
        --enable-transcoder-iconv 
	make -j $(JOBS) -C $(XERCES_BUILD)
		
install:
	make -C $(XERCES_BUILD) install

clean:
	rm -rf $(XERCES_OUTPUT)

xerces_prepare:
	if [ ! -d $(XERCES_SOURCE) ]; then \
		mkdir -p $(XERCES_SOURCE); \
		tar jxf $(XERCES_FILE) -C $(XERCES_OUTPUT); \
                chmod -R 755 ${XERCES_SOURCE} ; \
	fi

notice:
	cp -rf NOTICE.$(XERCES_VERSION).txt $(OSS_NOTICE_OUTPUT)/xerces_NOTICE.txt

info:
	cp -rf OSS_INFO.$(XERCES_VERSION).txt $(OSS_INFO_OUTPUT)/xerces_INFO.txt